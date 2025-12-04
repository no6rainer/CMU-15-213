/* 
 * trans.c - Matrix transpose B = A^T
 *
 * Each transpose function must have a prototype of the form:
 * void trans(int M, int N, int A[N][M], int B[M][N]);
 *
 * A transpose function is evaluated by counting the number of misses
 * on a 1KB direct mapped cache with a block size of 32 bytes.
 */ 
#include <stdio.h>
#include "cachelab.h"

int is_transpose(int M, int N, int A[N][M], int B[M][N]);
void transpose_8x8(int row, int col, int A[32][32], int B[32][32]);
void transpose_64x8(int col, int A[64][64], int B[64][64]);

/* 
 * transpose_submit - This is the solution transpose function that you
 *     will be graded on for Part B of the assignment. Do not change
 *     the description string "Transpose submission", as the driver
 *     searches for that string to identify the transpose function to
 *     be graded. 
 */
char transpose_submit_desc[] = "Transpose submission";
void transpose_submit(int M, int N, int A[N][M], int B[M][N])
{
    if (M == 32) {
        for (int i = 0; i < 4; ++i) {
            for (int j = 0; j < 4; ++j) {
                transpose_8x8(i, j, A, B);
            }
        }
    }

    if (M == 64) {
        for (int i = 0; i < 8; ++i) {
            transpose_64x8(i, A, B);
        }
    }

    if (M == 61) {

    }
}

void transpose_8x8(int row, int col, int A[32][32], int B[32][32]) {
    int v0, v1, v2, v3, v4, v5, v6, v7;
    for (int i = 0; i < 8; ++i) {
        v0 = A[row * 8 + i][col * 8 + 0];
        v1 = A[row * 8 + i][col * 8 + 1];
        v2 = A[row * 8 + i][col * 8 + 2];
        v3 = A[row * 8 + i][col * 8 + 3];
        v4 = A[row * 8 + i][col * 8 + 4];
        v5 = A[row * 8 + i][col * 8 + 5];
        v6 = A[row * 8 + i][col * 8 + 6];
        v7 = A[row * 8 + i][col * 8 + 7];

        B[col * 8 + 0][row * 8 + i] = v0;
        B[col * 8 + 1][row * 8 + i] = v1;
        B[col * 8 + 2][row * 8 + i] = v2;
        B[col * 8 + 3][row * 8 + i] = v3;
        B[col * 8 + 4][row * 8 + i] = v4;
        B[col * 8 + 5][row * 8 + i] = v5;
        B[col * 8 + 6][row * 8 + i] = v6;
        B[col * 8 + 7][row * 8 + i] = v7;
    }
}

void transpose_64x8(int col, int A[64][64], int B[64][64]) {
    int v0, v1, v2, v3, v4, v5, v6, v7;
    for (int i = 0; i < 56; ++i) {
        v0 = A[i][col * 8 + 0];
        v1 = A[i][col * 8 + 1];
        v2 = A[i][col * 8 + 2];
        v3 = A[i][col * 8 + 3];
        v4 = A[i][col * 8 + 4];
        v5 = A[i][col * 8 + 5];
        v6 = A[i][col * 8 + 6];
        v7 = A[i][col * 8 + 7];

        B[col * 8 + 0][i] = v0;
        B[col * 8 + 1][i] = v1;
        B[col * 8 + 2][i] = v2;
        B[col * 8 + 3][i] = v3;

        B[col * 8 + 4][i + 8] = v4;
        B[col * 8 + 5][i + 8] = v5;
        B[col * 8 + 6][i + 8] = v6;
        B[col * 8 + 7][i + 8] = v7;

        if (i % 8 == 7) {
            for (int j = i - 7; j < i + 1; ++j) {
                B[col * 8 + 4][j] = B[col * 8 + 4][j + 8];
                B[col * 8 + 5][j] = B[col * 8 + 5][j + 8];
                B[col * 8 + 6][j] = B[col * 8 + 6][j + 8];
                B[col * 8 + 7][j] = B[col * 8 + 7][j + 8];
            }      
        }
    }

    for (int i = 56; i < 60; ++i) {
        for (int j = col * 8; j < col * 8 + 4; ++j) {
            B[j][i] = A[i][j];
        }
    }
    for (int i = 56; i < 60; ++i) {
        for (int j = col * 8 + 4; j < col * 8 + 8; ++j) {
            B[j][i] = A[i][j];
        }
    }
    for (int i = 60; i < 64; ++i) {
        for (int j = col * 8; j < col * 8 + 4; ++j) {
            B[j][i] = A[i][j];
        }
    }
    for (int i = 60; i < 64; ++i) {
        for (int j = col * 8 + 4; j < col * 8 + 8; ++j) {
            B[j][i] = A[i][j];
        }
    }
}

/* 
 * You can define additional transpose functions below. We've defined
 * a simple one below to help you get started. 
 */ 

/* 
 * trans - A simple baseline transpose function, not optimized for the cache.
 */
char trans_desc[] = "Simple row-wise scan transpose";
void trans(int M, int N, int A[N][M], int B[M][N])
{
    int i, j, tmp;

    for (i = 0; i < N; i++) {
        for (j = 0; j < M; j++) {
            tmp = A[i][j];
            B[j][i] = tmp;
        }
    }    

}

/*
 * registerFunctions - This function registers your transpose
 *     functions with the driver.  At runtime, the driver will
 *     evaluate each of the registered functions and summarize their
 *     performance. This is a handy way to experiment with different
 *     transpose strategies.
 */
void registerFunctions()
{
    /* Register your solution function */
    registerTransFunction(transpose_submit, transpose_submit_desc); 


}

/* 
 * is_transpose - This helper function checks if B is the transpose of
 *     A. You can check the correctness of your transpose by calling
 *     it before returning from the transpose function.
 */
int is_transpose(int M, int N, int A[N][M], int B[M][N])
{
    int i, j;

    for (i = 0; i < N; i++) {
        for (j = 0; j < M; ++j) {
            if (A[i][j] != B[j][i]) {
                return 0;
            }
        }
    }
    return 1;
}

