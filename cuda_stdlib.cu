#define QUDA_GATE __device__
#define CUSTOM_HADAMARD
#define FOR_EACH_STATE(qreg, i) \
  for (i = blockIdx.x * blockDim.x + threadIdx.x; i < qreg->num_states; \
      i += blockDim.x * gridDim.x)
#define STATE(qreg, i) qreg->states[i]
#define AMPLITUDE(qreg, i) qreg->amplitudes[i]

#include "quantum_reg.h"
#include "complex.c"
typedef struct {
  int num_states;
  int size;
  int qubits;
  uint64_t *states;
  complex_t *amplitudes;
} cuda_quantum_reg;

#define quantum_reg cuda_quantum_reg
#include "quantum_gates.c"
#undef quantum_reg

// Magic for hadamard gate
__device__ int quda_quantum_hadamard_gate(int target, cuda_quantum_reg* qreg) {
	// If needed, enlarge qreg to make room for state splits resulting from this gate
  int states = qreg->num_states;
  if (2 * states > qreg->size) {
    *((volatile int *)0) = 0;
    return -1;
  }

	uint64_t mask = 1 << target;
	int i;
	FOR_EACH_STATE(qreg, i) {
		// Flipped state must be created
		STATE(qreg, qreg->num_states+i) = STATE(qreg, i) ^ mask;
		// For this state, must just modify amplitude
		AMPLITUDE(qreg, i) = quda_complex_rmul(AMPLITUDE(qreg, i),
				ONE_OVER_SQRT_2);
		// Copy amplitude to created state
		AMPLITUDE(qreg, qreg->num_states+i) = AMPLITUDE(qreg, i);

		if(STATE(qreg, i) & mask) {
			AMPLITUDE(qreg, i) = quda_complex_neg(AMPLITUDE(qreg, i));
		}
	}


  __syncthreads();
  if (blockIdx.x == 0 && threadIdx.x == 0)
  	qreg->num_states = 2*states;
  __syncthreads();

	return 0;
}

#include <stdio.h>
__global__ void cuda_quantum_fourier_kernel(cuda_quantum_reg *qreg) {
	int q = qreg->qubits-1;
	int i,j;
	for(i=q;i>=0;i--) {
		for(j=q;j>i;j--) {
			#ifdef QUDA_STDLIB_DEBUG
			printf("Performing c-R_%d (PI/%lu) on (%d,%d)\n",j-i+1,(uint64_t)1 << (j-i),j,i); // DEBUG
			#endif
			quda_quantum_controlled_rotate_k_gate(j,i,qreg,j-i+1);
		}
		#ifdef QUDA_STDLIB_DEBUG
		printf("Performing hadamard(bit %d)\n",i); // DEBUG
		#endif
		quda_quantum_hadamard_gate(i,qreg);
	}

	// TODO: Consider using SWAP gate here instead
	for(i=0;i<qreg->qubits/2;i++) {
		quda_quantum_controlled_not_gate(i,q-i,qreg);
		quda_quantum_controlled_not_gate(q-i,i,qreg);
		quda_quantum_controlled_not_gate(i,q-i,qreg);
	}
}

#define SANITY_CHECK(err) \
  do { \
    cudaError_t err__ = err; \
    if (err__ != cudaSuccess) { \
      fprintf(stderr, "Error at line %d: %s\n", __LINE__, \
          cudaGetErrorString(err__)); \
      exit(-1); \
    } \
  } while (0)
extern "C" void quda_cu_quantum_fourier_transform(quantum_reg* qreg) {
  uint64_t *states_device;
  complex_t *amplitudes_device;
  cuda_quantum_reg qreg_host, *qreg_device;
  cudaError_t err;

  // Copy over the states to the device
  qreg_host.size = 1 << qreg->qubits;
  qreg_host.num_states = qreg->num_states;
  qreg_host.qubits = qreg->qubits;
  err = cudaMalloc(&states_device, sizeof(uint64_t) * qreg_host.size);
  SANITY_CHECK(err);
  err = cudaMalloc(&amplitudes_device, sizeof(complex_t) * qreg_host.size);
  SANITY_CHECK(err);
  err = cudaMalloc(&qreg_device, sizeof(cuda_quantum_reg));
  SANITY_CHECK(err);

  // ... async!
  cudaStream_t stream;
  err = cudaStreamCreate(&stream);
  SANITY_CHECK(err);
  err = cudaMemcpy2DAsync(states_device, sizeof(uint64_t),
    &qreg->states[0].state, sizeof(uint64_t) + sizeof(complex_t),
    sizeof(uint64_t), qreg->num_states, cudaMemcpyHostToDevice, stream);
  SANITY_CHECK(err);
  err = cudaMemcpy2DAsync(amplitudes_device, sizeof(complex_t),
    &qreg->states[0].amplitude, sizeof(uint64_t) + sizeof(complex_t),
    sizeof(complex_t), qreg->num_states, cudaMemcpyHostToDevice, stream);
  SANITY_CHECK(err);
  qreg_host.states = states_device;
  qreg_host.amplitudes = amplitudes_device;

  // Copy over the device pointer for qreg
  err = cudaMemcpyAsync(qreg_device, &qreg_host, sizeof(cuda_quantum_reg),
    cudaMemcpyHostToDevice, stream);
  SANITY_CHECK(err);

  // Note for when we can stop waiting
  cudaEvent_t gate;
  err = cudaEventCreate(&gate);
  SANITY_CHECK(err);

  // Invoke the kernel
  dim3 localSize(128, 1, 1);
  dim3 globalSize(1, 1, 1);
  cuda_quantum_fourier_kernel<<<globalSize, localSize, 0, stream>>>(qreg_device);
  SANITY_CHECK(cudaGetLastError());
  // Free the memory locally (we'll replace it slightly later)
  err = cudaStreamWaitEvent(stream, gate, 0);
  SANITY_CHECK(err);
  err = cudaEventDestroy(gate);
  SANITY_CHECK(err);
  free(qreg->states);

  // Copy back the device pointer
  err = cudaMemcpyAsync(&qreg_host, qreg_device, sizeof(cuda_quantum_reg),
    cudaMemcpyDeviceToHost, stream);
  SANITY_CHECK(err);
  qreg->size = qreg->num_states = qreg_host.num_states;
  // ... and the states
  qreg->states = (quantum_state_t*)malloc(sizeof(quantum_state_t) * qreg->num_states);
  err = cudaMemcpy2DAsync(&qreg->states[0].state,
    sizeof(uint64_t) + sizeof(complex_t),
    states_device, sizeof(uint64_t),
    sizeof(uint64_t), qreg->num_states, cudaMemcpyDeviceToHost, stream);
  SANITY_CHECK(err);
  err = cudaMemcpy2DAsync(&qreg->states[0].amplitude,
    sizeof(uint64_t) + sizeof(complex_t),
    amplitudes_device, sizeof(complex_t),
    sizeof(complex_t), qreg->num_states, cudaMemcpyDeviceToHost, stream);
  SANITY_CHECK(err);

  err = cudaStreamSynchronize(stream);
  SANITY_CHECK(err);
  err = cudaStreamDestroy(stream);
  SANITY_CHECK(err);
}
