/* Copyright © 2014 Intel Corporation
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */

/** @file ranges-2.c
 *
 * Test that *just* changing the bound range of a TexBO (without changing
 * anything else) works. This is to demonstrate a bug in Mesa's dirty state
 * flagging.
 */

#include "piglit-util-gl.h"

PIGLIT_GL_TEST_CONFIG_BEGIN

        config.supports_gl_core_version = 31;

        config.window_visual = PIGLIT_GL_VISUAL_DOUBLE | PIGLIT_GL_VISUAL_RGBA;
	config.khr_no_error_support = PIGLIT_NO_ERRORS;

PIGLIT_GL_TEST_CONFIG_END

GLuint prog;
GLuint vao;
GLuint tbo;
GLuint tex;

char const *vs_source =
"#version 140\n"
"uniform samplerBuffer s;\n"
"out vec4 color;\n"
"void main() {\n"
"	vec4 x = texelFetch(s, gl_VertexID);\n"
"	gl_Position = vec4(x.xy, 0, 1);\n"
"	color = vec4(x.zw, 0, 1);\n"
"}\n";

char const *fs_source =
"#version 140\n"
"in vec4 color;\n"
"out vec4 frag_color;\n"
"void main() {\n"
"	frag_color = color;\n"
"}\n";

float data[] = {
	-1, -1,		0, 1,
	0, -1,		0, 1,
	0, 0,		0, 1,
	-1, -1,		0, 1,
	0, 0,		0, 1,
	-1, 0,		0, 1,

	-1, 0,		0, 0.5,
	0, 0,		0, 0.5,
	0, 1,		0, 0.5,
	-1, 0,		0, 0.5,
	0, 1,		0, 0.5,
	-1, 1,		0, 0.5,

	0, 0,		1, 0,
	1, 0,		1, 0,
	1, 1,		1, 0,
	0, 0,		1, 0,
	1, 1,		1, 0,
	0, 1,		1, 0,

	0, -1,		0.5, 0,
	1, -1,		0.5, 0,
	1, 0,		0.5, 0,
	0, -1,		0.5, 0,
	1, 0,		0.5, 0,
	0, 0,		0.5, 0,
};

int aligned_size = 0;
int chunk_size = 24 * sizeof(float);
int num_chunks = 4;

enum piglit_result
piglit_display(void) {
	int i;
	bool pass = true;

	glClearColor(0.2, 0.2, 0.2, 0.2);
	glClear(GL_COLOR_BUFFER_BIT);

	/* verify unaligned offsets produce an error */
	glTexBufferRange(GL_TEXTURE_BUFFER, GL_RGBA32F, tbo, aligned_size - 1, 1);
	pass &= glGetError() == GL_INVALID_VALUE;

	for (i = 0; i < num_chunks; i++) {
		glTexBufferRange(GL_TEXTURE_BUFFER, GL_RGBA32F,
				 tbo, i * aligned_size, chunk_size);
		glDrawArrays(GL_TRIANGLES, 0, 6);
	}

	for (i = 0; i < num_chunks; i++) {
		float c[4] = {
			data[i * 24 + 2],
			data[i * 24 + 3],
			0,
			1
		};

		pass &= piglit_probe_rect_rgba(
			piglit_width * 0.5 * (1 + data[i * 24 + 0]),
			piglit_height * 0.5 * (1 + data[i * 24 + 1]),
			piglit_width/2,
			piglit_height/2, c) && pass;
	}

	piglit_present_results();

	return pass ? PIGLIT_PASS : PIGLIT_FAIL;
}

void
piglit_init(int argc, char **argv) {
	GLint align, i;
	uint8_t *chunk;

	piglit_require_extension("GL_ARB_texture_buffer_range");

	glGetIntegerv(GL_TEXTURE_BUFFER_OFFSET_ALIGNMENT, &align);
	aligned_size = ALIGN(chunk_size, align);

	prog = piglit_build_simple_program(vs_source, fs_source);
	glUseProgram(prog);

	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);

	glGenBuffers(1, &tbo);
	glBindBuffer(GL_ARRAY_BUFFER, tbo);
	glBufferData(GL_ARRAY_BUFFER, aligned_size * num_chunks, NULL, GL_STATIC_DRAW);

	for (i = 0, chunk = (uint8_t *)data; i < num_chunks; i++) {
		glBufferSubData(GL_ARRAY_BUFFER, aligned_size * i, chunk_size, chunk);
		chunk += chunk_size;
	}

	glGenTextures(1, &tex);
	glBindTexture(GL_TEXTURE_BUFFER, tex);
}
