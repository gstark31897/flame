#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <strings.h>
#include <string.h>

#include <pthread.h>

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#define VARIANT_COUNT 3
#define POINT_COUNT 2000
#define ITERATIONS 500
#define AFFINE_COUNT 5
#define WIDTH 800
#define HEIGHT 800
#define THREADS 4

static unsigned int g_seed;

inline void fast_srand(int seed)
{
	srand(seed);
	g_seed = seed;
	g_seed = rand();
};

inline int fast_rand(void)
{
	g_seed = (214013*g_seed+2531011);
	return (g_seed>>16)&0x7FFF;
};

void operate(float *p, float r, float omega, float theta, int v)
{
	float c = 0.0f;
	float tx, ty;
	switch (v)
	{
	case 0:
		return;
	case 1:
		p[0] = sinf(p[0]);
		p[1] = sinf(p[1]);
		return;
	case 2:
		c = r*r;
		tx = p[0]*sinf(c) - p[1]*cosf(c);
		ty = p[0]*cosf(c) - p[1]*sinf(c);
		p[0] = tx;
		p[1] = ty;
		return;
	case 3:
		c = 1.0/(r*r);
		p[0] = p[0] * c;
		p[1] = p[1] * c;
		return;
	case 4:
		c = sqrtf(r);
		p[0] = c*cosf(theta/2.0 + omega);
		p[1] = c*sinf(theta/2.0 + omega);
		return;
	case 5:
		ty = p[0];
		p[0] = p[1];
		p[1] = ty;
		return;
	default :
		return;
	}
}

void affine(float *p, float *affine, float i)
{
	float nx = p[0]*affine[0] + p[1]*affine[1] + affine[2];
	float ny = p[0]*affine[3] + p[1]*affine[4] + affine[5];
	float nz = p[2] + (affine[6]- p[2])/(i*i+1.0f);
	p[0] = nx; p[1] = ny; p[2] = affine[6];
}

int process(float *points, float *affines, float *output, int i)
{
	int ac, vc;
	float r, omega, theta;
	float *p, *o;
	for (int j = 0; j < POINT_COUNT * 3; j += 3)
	{
		p = points + j;
		o = output + POINT_COUNT*i*3 + j;
		ac = (fast_rand()%AFFINE_COUNT)*7;
		vc = fast_rand()%VARIANT_COUNT;
		r = sqrtf(p[0]*p[0] + p[1]*p[1]);
		omega = ((float)fast_rand())/((float)RAND_MAX) * M_PI;
		theta = atanf(p[0]/p[1]);
		affine(p, affines+ac, (float)i);
		operate(p, r, omega, theta, vc);
		o[0] = p[0];
		o[1] = p[1];
		o[2] = p[2];
	}
}

char *getFile(const char *path)
{
	FILE *f = fopen(path, "rb");
	fseek(f, 0, SEEK_END);
	long size = ftell(f);
	fseek(f, 0, SEEK_SET);

	char *data = malloc(sizeof(char) * (size + 1));
	fread(data, 1, size, f);
	fclose(f);
	data[size] = 0;

	return data;
}

GLuint loadShaders(const char *vertex_path, const char *fragment_path)
{
	GLuint vertexShaderID = glCreateShader(GL_VERTEX_SHADER);
	GLuint fragmentShaderID = glCreateShader(GL_FRAGMENT_SHADER);

	char *vertexSrc = getFile(vertex_path);
	char *fragmentSrc = getFile(fragment_path);

	GLint result = GL_FALSE;
	int logLength;

	glShaderSource(vertexShaderID, 1, &vertexSrc, NULL);
	glCompileShader(vertexShaderID);
	glGetShaderiv(vertexShaderID, GL_COMPILE_STATUS, &result);
	glGetShaderiv(vertexShaderID, GL_INFO_LOG_LENGTH, &logLength);
	if (logLength > 0)
	{
		char *log = malloc(sizeof(char) * (logLength + 1));
		glGetShaderInfoLog(vertexShaderID, logLength, NULL, log);
		printf("%s\n", log);
		free(log);
	}

	glShaderSource(fragmentShaderID, 1, &fragmentSrc, NULL);
	glCompileShader(fragmentShaderID);
	glGetShaderiv(fragmentShaderID, GL_COMPILE_STATUS, &result);
	glGetShaderiv(fragmentShaderID, GL_INFO_LOG_LENGTH, &logLength);
	if (logLength > 0)
	{
		char *log = malloc(sizeof(char) * (logLength + 1));
		glGetShaderInfoLog(fragmentShaderID, logLength, NULL, log);
		printf("%s\n", log);
		free(log);
	}

	GLuint programID = glCreateProgram();
	glAttachShader(programID, vertexShaderID);
	glAttachShader(programID, fragmentShaderID);
	glLinkProgram(programID);

	glGetProgramiv(programID, GL_LINK_STATUS, &result);
	glGetProgramiv(programID, GL_INFO_LOG_LENGTH, &logLength);
	if (logLength > 0)
	{
		char *log = malloc(sizeof(char) * (logLength + 1));
		glGetProgramInfoLog(programID, logLength, NULL, log);
		printf("%s\n", log);
		free(log);
	}

	glDetachShader(programID, vertexShaderID);
	glDetachShader(programID, fragmentShaderID);

	glDeleteShader(vertexShaderID);
	glDeleteShader(fragmentShaderID);

	free(vertexSrc);
	free(fragmentSrc);

	return programID;
}

void *render_thread(void **data)
{
	float *points = (float*)data[0];
	float *original_points = (float*)data[1];
	float *affines = (float*)data[2];
	float *output = (float*)data[3];

	memcpy(points, original_points, POINT_COUNT*3);
	// Render
	for (int i = 0; i < ITERATIONS; ++i)
	{
		process(points, affines, output, i);
	}
}

#define PARAMS 4
void render(float *points, float *original_points, float *affines, float *output)
{
	void *data[THREADS*PARAMS];

	pthread_t threads[THREADS];
	int results[THREADS];
	for (int i = 0; i < THREADS; ++i)
	{
		data[i*PARAMS] = points + i*POINT_COUNT*3;
		data[i*PARAMS+1] = original_points + i*POINT_COUNT*3;
		data[i*PARAMS+2] = affines;
		data[i*PARAMS+3] = output + i*POINT_COUNT*3;
		pthread_create(&threads[i], NULL, render_thread, data+i*PARAMS);
	}

	for (int i = 0; i < THREADS; ++i)
	{
		pthread_join(threads[i], NULL);
	}
}

int main()
{
	// Init
	float *points = malloc(sizeof(float) * POINT_COUNT * 3 * THREADS);
	float *original_points = malloc(sizeof(float) * POINT_COUNT * 3 * THREADS);
	float *affines = malloc(sizeof(float) * AFFINE_COUNT * 7);
	float *output = malloc(sizeof(float) * POINT_COUNT * 3 * ITERATIONS * THREADS);

	fast_srand(time(NULL));

	printf("Making point cloud\n");
	for (int i = 0; i < POINT_COUNT * 3 * THREADS; i += 3)
	{
		original_points[i] = ((float)(rand() % 10000))/10000.0f * 2.0f - 1.0f;
		original_points[i+1] = ((float)(rand() % 10000))/10000.0f * 2.0f - 1.0f;
		original_points[i+2] = 0.0f;
	}

	for (int i = 0; i < AFFINE_COUNT * 7; i++)
	{
		affines[i] = ((float)(rand() % 10000))/10000.0f * 2.0f - 1.0f;
		if ((i+1) % 7 == 0)
		{
			affines[i] = (float)(rand() % 10000)/10000.0f;
		}
		printf("affine: %f\n", affines[i]);
	}
	
	// GL
	if (!glfwInit())
	{
		printf("Failed to initialize GLFW\n");
		return -1;
	}

	//glfwWindowHint(GLFW_SAMPLES, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	GLFWwindow *window = glfwCreateWindow(900, 900, "flame", NULL, NULL);
	if (window == NULL)
	{
		printf("Failed to open GLFW window\n");
		glfwTerminate();
		return -1;
	}
	glfwMakeContextCurrent(window);
	if (glewInit() != GLEW_OK)
	{
		printf("Failed to initialize GLEW\n");
		return -1;
	}
	glfwSetInputMode(window, GLFW_STICKY_KEYS, GL_TRUE);

	/*GLfloat *ps = malloc(sizeof(GLfloat) * 3 * WIDTH * HEIGHT);
	int i = 0;
	for (long y = 0; y < HEIGHT; y++)
	{
		for (long x = 0; x < WIDTH; x++)
		{
			float *p = ps + i;
			p[0] = ((float)x)/((float)WIDTH)*2.0f-1.0f;
			p[1] = ((float)y)/((float)HEIGHT)*2.0f-1.0f;
			p[2] = 0.0f;
			i += 3;
		}
	}*/

	GLuint vaID;
	glGenVertexArrays(1, &vaID);
	glBindVertexArray(vaID);

	GLuint vertexbuffer;
	glGenBuffers(1, &vertexbuffer);
	glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat)*POINT_COUNT*3*ITERATIONS*THREADS, output, GL_STATIC_DRAW);

	float max = 0.0;

	GLuint shader = loadShaders("shader.vert", "shader.frag");
	GLuint maxLoc = glGetUniformLocation(shader, "maxValue");
	glUseProgram(shader);
	glPointSize(1);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnableVertexAttribArray(0);

	do
	{
		glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);

		affines[0] += 0.01;
		affines[8] += 0.005;
		affines[17] -= 0.015;
		render(points, original_points, affines, output);
		glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat)*POINT_COUNT*3*ITERATIONS*THREADS, output, GL_STATIC_DRAW);
		glUniform1f(maxLoc, 10000.0f);

		glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, NULL);
		glDrawArrays(GL_POINTS, 0, POINT_COUNT*ITERATIONS*THREADS);

		glfwSwapBuffers(window);
		glfwPollEvents();
	}
	while (glfwGetKey(window, GLFW_KEY_ESCAPE) != GLFW_PRESS &&
			glfwWindowShouldClose(window) == 0);
	glDisableVertexAttribArray(0);

	//free(points);
}
