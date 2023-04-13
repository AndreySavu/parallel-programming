#include <omp.h>
#include <iostream>
#include <math.h>
#include <chrono>
using namespace std;
#define eps 1e-5

// Метод Гаусса-Зейделя
double* Gauss_S(double **A, double *B, int n)
{
	int iterations_count = 0;
	double norm, s;
	double *x = new double[n];
	double *prev_x = new double[n];
	#pragma omp parallel for
	for (int i = 0; i < n; i++)
        x[i] = 0;

    do
    {   
    	#pragma omp parallel for
    	for (int i = 0; i < n; i++)
        {
        	prev_x[i] = x[i];
        	s = 0;
        	#pragma omp parallel for reduction(+ : s)
        	for (int j = 0; j < n; j++) 
        		if (j != i) 
        			s += (A[i][j] * x[j]);
        	x[i] = (B[i] - s);
        	x[i] /=  A[i][i];
        }
        norm = 0;
        #pragma omp parallel for reduction(+ : norm)
        for (int i = 0; i < n; i++)
        	norm += (x[i] - prev_x[i]) * (x[i] - prev_x[i])/(prev_x[i]*prev_x[i]);
        
        iterations_count++;

    } while (!(sqrt(norm) <= eps));

    return x;
}

// argv[1]-число потоков, argv[2]-размер системы
int main(int argc, char *argv[])
{
    int threads_count = atoi(argv[1]);
    int size = atoi(argv[2]);

    omp_set_num_threads(threads_count);

    double **A = new double *[size];
    double *B = new double[size];
    double *x = new double[size];
	#pragma omp parallel for
    for (int i = 0; i < size; i++)
    {
        A[i] = new double[size];
        B[i] = 0;
        for (int j = 0; j < size; j++)
        {
            A[i][j] = 1.0 / pow(3, abs(i - j));
            B[i] += A[i][j];
        }
    }
	auto begin = chrono::steady_clock::now();
    x = Gauss_S(A, B, size);
	auto elapsed_time = chrono::duration_cast<chrono::microseconds>(chrono::steady_clock::now() - begin);
	cout << "Time = " << elapsed_time.count() << '\n';
    
	/*
	cout.fill(' ');
	for (int i = 0; i < size; i++)
	{
		cout << '|';
		for (int j = 0; j < size; j++)
		{
			cout.width(12);
			cout << A[i][j] << " ";
        }
		cout << '|';
		cout.width(10);
		cout << B[i] << endl;
	}

	cout << "X= (";
	for (int i = 0; i < size; i++)
		cout << x[i] << ", ";
	cout << ")\n";
	*/
    for (int i = 0; i < size; i++)
        delete A[i];
    delete A;
    delete B;
    delete x;
}
