#define QUDA_GATE __device__
#define CUSTOM_HADAMARD
#define FOR_EACH_STATE(qreg, i) \
  for (i = blockIdx.x * blockDim.x + threadIdx.x; i < qreg->num_states; \
      i += blockDim.x * gridDim.x)
#include "quantum_reg.h"
#include "complex.c"
#include "quantum_gates.c"

// Magic for hadamard gate
__device__ int quda_quantum_hadamard_gate(int target, quantum_reg* qreg) {
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
		qreg->states[qreg->num_states+i].state = qreg->states[i].state ^ mask;
		// For this state, must just modify amplitude
		qreg->states[i].amplitude = quda_complex_rmul(qreg->states[i].amplitude,
				ONE_OVER_SQRT_2);
		// Copy amplitude to created state
		qreg->states[qreg->num_states+i].amplitude = qreg->states[i].amplitude;

		if(qreg->states[i].state & mask) {
			qreg->states[i].amplitude = quda_complex_neg(qreg->states[i].amplitude);
		}
	}

  __syncthreads();
  if (blockIdx.x == 0 && threadIdx.x == 0)
  	qreg->num_states = 2*states;
  __syncthreads();

	// TODO: Ideally, make this call optional or conditional
	quda_quantum_reg_coalesce(qreg);

	return 0;
}
__device__ void quda_quantum_reg_coalesce(quantum_reg *qreg) {
  __syncthreads();
}

__global__ void cuda_quantum_fourier_kernel(quantum_reg *qreg) {
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

#include <stdio.h>
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
  quantum_state_t *qstates_device;
  quantum_reg qreg_host = *qreg, *qreg_device;
  cudaError_t err;

  // Copy over the states to the device
  err = cudaMalloc(&qstates_device, 1LL << qreg->size);
  SANITY_CHECK(err);
  err = cudaMalloc(&qreg_device, sizeof(quantum_reg));
  SANITY_CHECK(err);
  err = cudaMemcpy(qstates_device, qreg->states,
    qreg->num_states * sizeof(quantum_state_t), cudaMemcpyHostToDevice);
  SANITY_CHECK(err);
  qreg_host.states = qstates_device;

  // Copy over the device pointer for qreg
  err = cudaMemcpy(qreg_device, &qreg_host, sizeof(quantum_reg),
    cudaMemcpyHostToDevice);
  SANITY_CHECK(err);

  // Invoke the kernel
  dim3 localSize(128, 1, 1);
  dim3 globalSize(1, 1, 1);
  cuda_quantum_fourier_kernel<<<globalSize, localSize>>>(qreg_device);
  SANITY_CHECK(cudaGetLastError());
  // Free the memory locally (we'll replace it slightly later)
  free(qreg->states);

  // Copy back the device pointer
  err = cudaMemcpy(qreg, qreg_device, sizeof(quantum_reg),
    cudaMemcpyDeviceToHost);
  SANITY_CHECK(err);
  // ... and the states
  qreg->states = (quantum_state_t*)malloc(sizeof(quantum_state_t) * qreg->num_states);
  err = cudaMemcpy(qreg->states, qstates_device,
    sizeof(quantum_state_t) * qreg->num_states, cudaMemcpyDeviceToHost);
  SANITY_CHECK(err);
}
