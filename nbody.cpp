#include <iostream>
#include <stdio.h>
#include <string.h>
#include <iomanip>
#include <omp.h>
#include <cmath>
#include <sys/time.h>
#include <fstream>
#include <pthread.h>

using namespace std;
#define max_size 1024

#define GET_TIME(now){ 	\
	struct timeval t; 	\
	gettimeofday(&t,nullptr);	\
	now = t.tv_sec + t.tv_usec / 1000000.0;	\
}

double dt = 0.005; //最小模拟时间
double G = 1;	   //万有引力系数


struct planet // 计算使用结构体，记录下body状态
{
	double mass;	// body质量
	double sx, sy, sz;	// body三个维度速度
	double x, y, z;		// body三个维度位置
	double fx, fy, fz;	// body三个维度力大小
	planet()
	{
		mass = sx = sy = sz = x = y = z = 0;
		fx = fy = fz = 0;
	}
};
struct thread_set // pthread参数传递
{
	int rank;		// 线程表示号
	planet *o;		// 指向计算数据指针
};

void print(planet o)
{
	cout << o.mass << ' '
		 << o.x << ' ' << o.y << ' ' << o.z
		 << ' ' << o.sx << ' ' << o.sy << ' ' << o.sz << endl;
}

void speed_change(planet &a, planet &b)	// 更新两个body的三维力大小数据
{
	double dis = sqrt(pow(a.x - b.x, 2) + pow(a.y - b.y, 2) + pow(a.z - b.z, 2));
	double F = G * a.mass * b.mass / pow(dis, 2);
	double xf = (a.x - b.x) / dis;
	double yf = (a.y - b.y) / dis;
	double zf = (a.z - b.z) / dis;
	a.fx -= F * xf;
	a.fy -= F * yf;
	a.fz -= F * zf;
	b.fx += F * xf;
	b.fy += F * yf;
	b.fz += F * zf;
}
void postion(planet &a)					// 根据当前body的收到总力和速度更新速度和位置
{
	a.sx += a.fx / a.mass * dt;
	a.sy += a.fy / a.mass * dt;
	a.sz += a.fz / a.mass * dt;
	a.fx = a.fy = a.fz = 0;
	a.x += a.sx * dt;
	a.y += a.sy * dt;
	a.z += a.sz * dt;
}

void read(planet *array)				// 从文件中读取数据并且存放到合适的位置
{
	fstream fp;
	fp.open("nbody.txt", ios::in);
	if (fp.bad())
	{
		cout << "error to open file";
	}
	for (int i = 0; i < max_size; i++)
	{
		fp >> array[i].mass >> array[i].x >> array[i].y >> array[i].z >> array[i].sx >> array[i].sy >> array[i].sz;
	}
	fp.close();
}

void write(planet *array, const char *file)			// 向目的文件写入计算结果
{
	fstream fp;
	fp.open(file, ios::out);
	if (fp.bad())
	{
		cout << "output open error";
	}
	for (int i = 0; i < max_size; i++)
	{
		fp << setprecision(15) << setiosflags(ios::left) << array[i].mass << " " << array[i].x << ' '
		   << array[i].y << ' ' << array[i].z << ' '
		   << array[i].sx << ' ' << array[i].sy << ' ' << array[i].sz << endl;
	}
	fp.close();
}

void parallel_stimulus1(unsigned int count, planet *array)	// openmp并行程序
{
	int i, j;
	while (count--)
	{

#pragma omp parallel for num_threads(4) private(i, j) shared(array)
		for (i = 0; i < max_size; i++)
		{
			for (j = i + 1; j < max_size; j++)
			{
				speed_change(array[i], array[j]);
			}
		}
#pragma omp parallel for num_threads(8) private(i) shared(array)
		for (i = 0; i < max_size; i++)
		{
			postion(array[i]);
		}
	}
}

void *thread_pthread(void *argv)							// pthread的计算线程
{
	thread_set set = *(thread_set *)argv;
	long rank = set.rank;
	int n = 0;
	
	if (rank == 3)
	{
		n = max_size - 1;
	}
	else
	{
		n = (rank + 1) * max_size / 4;						// 如果下面修改了线程数量，这里也需要一并修改。
	}

	for (int i = rank * max_size / 4; i < n; i++)
	{
		for (int j = i + 1; j <= max_size; j++)
		{
			speed_change(set.o[i], set.o[j]);
		}
	}

	return nullptr;
}
void parallel_stimulus2(unsigned int count, planet *array)		// pthread并行程序
{
	int num_of_thread = 4;						// 线程数量
	thread_set *set = new thread_set[num_of_thread];

	pthread_t *thread_handle = new pthread_t[num_of_thread];
	while (count--)
	{
		for (long i = 0; i < num_of_thread; i++)
		{
			set[i].rank = i;
			set[i].o = array;
			pthread_create(&thread_handle[i], nullptr, thread_pthread, (void *)&set[i]);
		}
		for (long i = num_of_thread-1; i >= 0; i--)
		{
			pthread_join(thread_handle[i], nullptr);
		}
		
		for (int i = 0; i < max_size; i++)
		{
			postion(array[i]);
		}
	}
	delete[] thread_handle;
	delete[] set;
	return;
}

void series_stimulus(unsigned int count, planet *array)				// 唯一串行程序
{
	while (count--)
	{
		// start = clock();
		for (int i = 0; i < max_size; i++)
		{
			// postion(array[i]);
			for (int j = i + 1; j < max_size; j++)
			{
				speed_change(array[i], array[j]);
			}
		}
		for (int i = 0; i < max_size; i++)
		{
			postion(array[i]);
		}
	}
}

int main()
{
	double start, end;
	planet array[max_size + 16];
	planet copy_planet[max_size + 16];
	read(array);

	unsigned int count = 20; //迭代20次
	char series[] = "series version.txt", parallel1[] = "parallel version1.txt", parallel2[] = "parallel version2.txt";

	memcpy(copy_planet, array, max_size * sizeof(planet)); // 串行程序
	GET_TIME(start);
	series_stimulus(count, copy_planet);
	GET_TIME(end)
	cout << (double)(end - start) << endl;
	// 要写出如文件请去掉本行注释，下面同理
	// write(copy_planet, series);

	memcpy(copy_planet, array, max_size * sizeof(planet)); // openmp程序
	GET_TIME(start);
	parallel_stimulus1(count, copy_planet);
	GET_TIME(end)
	cout << (double)(end - start) << endl;
	// write(copy_planet, parallel1);

	memcpy(copy_planet, array, max_size * sizeof(planet)); // pthread程序
	GET_TIME(start);
	parallel_stimulus2(count, copy_planet);
	GET_TIME(end)
	cout << (double)(end - start) / CLOCKS_PER_SEC << endl;
	// write(copy_planet, parallel2);
}
