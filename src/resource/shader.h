/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information. 
 * ---
 * Copyright (C) 2011, Lukas Weber <laochailan@web.de>
 */

#ifndef SHADER_H
#define SHADER_H

#include <GL/glew.h>

#define DELIM "%% -- FRAG"
#define DELIM_SIZE 10

struct Uniform;
typedef struct Uniform {
	char *name;
	GLint location;
} Uniform;

struct Shader;
typedef struct Shader {
	struct Shader *next;
	struct Shader *prev;
	
	char *name;
	GLuint prog;
	
	int unicount;
	Uniform *uniforms;
} Shader;

void load_shader(const char *filename);
Shader *get_shader(const char *name);
void delete_shaders();

void cache_uniforms(Shader *sha);
int uniloc(Shader *sha, char *name);
#endif