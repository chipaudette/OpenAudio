// https://forum.pjrc.com/threads/27959-FLOPS-not-scaling-to-F_CPU
#include <math.h>
#include <string.h>

#include <arm_math.h>

//FASTRUN
void float_MatMult(float* A, float* B, int m, int p, int n, float* C) {
  // A = input matrix (m x p)
  // B = input matrix (p x n)
  // m = number of rows in A
  // p = number of columns in A = number of rows in B
  // n = number of columns in B
  // C = output matrix = A*B (m x n)
  int i, j, k;
  for ( i = 0; i < m; i++ )
    for ( j = 0; j < n; j++ ){
      C[i*n+j] = 0;
      for( k = 0; k < p; k++ )  C[i*n+j] += A[i*p+k]*B[k*n+j];
    }
}

#define N 128
  float32_t A[N][N];
  float32_t B[N][N];
  float32_t C[N][N];


void setup() {
  
  while (!Serial) ;
  
  // variables for timing
  int i=0;
  int dt;
  
  // variables for calculation


  memset(A,3.1415,sizeof(A));
  memset(B,8.1415,sizeof(B));
  memset(C,0.0,sizeof(C));

  arm_matrix_instance_f32 A_inst, B_inst, C_inst;
  arm_mat_init_f32(&A_inst, N, N, (float32_t *)A);
  arm_mat_init_f32(&B_inst, N, N, (float32_t *)B);
  arm_mat_init_f32(&C_inst, N, N, (float32_t *)C);
  
  int tbegin = micros();
#if 1
  for (i=1;;i++) {
    // do calculation
    //float_MatMult((float*) A, (float*)B, N,N,N, (float*)C);
    arm_mat_mult_f32(&A_inst, &B_inst, &C_inst);
    // check if t_delay has passed
    dt = micros() - tbegin;
    if (dt > 1000000) break;
  }
#else
  for (i=0; i < 200; i++) {
    float_MatMult((float*) A, (float*)B, N,N,N, (float*)C);
  }
  dt = micros() - tbegin;
#endif

  Serial.printf("(%dx%d) matrices: ", N, N);
  Serial.printf("%d matrices in %d usec: ", i, dt);
  Serial.printf("%d matrices/second\n", (int)((float)i*1000000/dt));
  Serial.printf("Float (%d bytes) ", sizeof(float));
  //float total = N*N*N*i*1e6 / (float)dt;
  float total_MFLOPS = N*N*N*i / (float)dt;
  Serial.printf(" MFLOPS:\t(%f)\n", total_MFLOPS);

  ///////// Vector multiply
 tbegin = micros();
  for (i=1;;i++) {
    // do calculation
    //float_MatMult((float*) A, (float*)B, N,N,N, (float*)C);
    arm_mult_f32((float32_t *)A, (float32_t *)B, (float32_t *)C, N*N);
    // check if t_delay has passed
    dt = micros() - tbegin;
    if (dt > 1000000) break;
  }

  Serial.printf("(%dx) Vector Multiply: ",N*N);
  Serial.printf("%d vectors in %d usec: ", i, dt);
  Serial.printf("%d vectors/second\n", (int)((float)i*1000000/dt));
  Serial.printf("Float (%d bytes) ", sizeof(float));
  //float total = N*i*1e6 / (float)dt;
  total_MFLOPS = N*N*i / (float)dt;
  Serial.printf(" MFLOPS:\t(%f)\n", total_MFLOPS);


  ///////// FIR
  arm_fir_instance_f32 FIR_inst;
  #define N_FIR (4*N)
  #define N_BUFF (16*N_FIR)
  arm_fir_init_f32(&FIR_inst, N_FIR, (float32_t *)B[0], (float32_t *)B[N_BUFF/N_FIR],N_BUFF);
  tbegin = micros();
  for (i=1;;i++) {
    // do calculation
    //float_MatMult((float*) A, (float*)B, N,N,N, (float*)C);
    arm_fir_f32(&FIR_inst,(float32_t *)A, (float32_t *)C,N_BUFF);
    // check if t_delay has passed
    dt = micros() - tbegin;
    if (dt > 1000000) break;
  }


  Serial.printf("(%dx) FIR on %d buffer: ",N_FIR,N_BUFF);
  Serial.printf("%d buffers in %d usec: ", i, dt);
  Serial.printf("%d Buffers/second\n", (int)((float)i*1000000/dt));
  Serial.printf("Float (%d bytes) ", sizeof(float));
  //float total = N*i*1e6 / (float)dt;
  total_MFLOPS = N_FIR*N_BUFF*i / (float)dt;
  Serial.printf(" MFLOPS:\t(%f)\n", total_MFLOPS);
  
}

void loop() {
}
