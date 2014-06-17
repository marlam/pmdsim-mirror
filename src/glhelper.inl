/*
 * Copyright (C) 2012, 2013, 2014
 * Computer Graphics Group, University of Siegen, Germany.
 * http://www.cg.informatik.uni-siegen.de/
 * All rights reserved.
 * Written by Martin Lambers <martin.lambers@uni-siegen.de>.
 */

#ifdef NDEBUG
# define XGL_HERE ""
#else
# define XGL_STRINGIFY(x) XGL_STRINGIFY_(x)
# define XGL_STRINGIFY_(x) #x
# define XGL_HERE (__FILE__ ":" XGL_STRINGIFY(__LINE__))
#endif

bool xglCheckFBO(const std::string& where = "")
{
    GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (status != GL_FRAMEBUFFER_COMPLETE) {
        std::string pfx = (where.length() > 0 ? where + ": " : "");
        std::fprintf(stderr, "%sOpenGL FBO error 0x%04X\n", pfx.c_str(), status);
        std::exit(1);
    }
    return true;
}

bool xglCheckError(const std::string& where = "")
{
    GLenum e = glGetError();
    if (e != GL_NO_ERROR) {
        std::string pfx = (where.length() > 0 ? where + ": " : "");
        std::fprintf(stderr, "%sOpenGL error 0x%04X\n", pfx.c_str(), e);
        std::exit(1);
    }
    return true;
}

void xglKillCrlf(std::string& str)
{
    size_t l = str.length();
    if (l > 0 && str[l - 1] == '\n')
        str[--l] = '\0';
    if (l > 0 && str[l - 1] == '\r')
        str[--l] = '\0';
}

GLuint xglCompileShader(GLenum type, const char* src, const std::string& where = "")
{
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &src, NULL);
    glCompileShader(shader);

    std::string log;
    GLint e, l;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &e);
    glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &l);
    if (l > 0) {
        char* tmplog = new char[l];
        glGetShaderInfoLog(shader, l, NULL, tmplog);
        log = tmplog;
        delete[] tmplog;
        xglKillCrlf(log);
    }

    if (e == GL_TRUE && log.length() > 0) {
#ifndef NDEBUG
        std::string pfx = (where.length() > 0 ? where + ": " : "");
        std::fprintf(stderr, "%sOpenGL compiler warning: %s\n", pfx.c_str(), log.c_str());
#endif
    } else if (e != GL_TRUE) {
        std::string pfx = (where.length() > 0 ? where + ": " : "");
        std::fprintf(stderr, "%sOpenGL compiler error: %s\n", pfx.c_str(), log.c_str());
        std::exit(1);
    }
    return shader;
}

GLuint xglCreateProgram(GLuint vshader, GLuint gshader, GLuint fshader)
{
    GLuint program = glCreateProgram();
    if (vshader != 0)
        glAttachShader(program, vshader);
    if (gshader != 0)
        glAttachShader(program, gshader);
    if (fshader != 0)
        glAttachShader(program, fshader);
    return program;
}

void xglLinkProgram(const GLuint prg, const std::string& where = "")
{
    glLinkProgram(prg);

    std::string log;
    GLint e, l;
    glGetProgramiv(prg, GL_LINK_STATUS, &e);
    glGetProgramiv(prg, GL_INFO_LOG_LENGTH, &l);
    if (l > 0) {
        char* tmplog = new char[l];
        glGetProgramInfoLog(prg, l, NULL, tmplog);
        log = tmplog;
        delete[] tmplog;
        xglKillCrlf(log);
    }

    if (e == GL_TRUE && log.length() > 0) {
#ifndef NDEBUG
        std::string pfx = (where.length() > 0 ? where + ": " : "");
        std::fprintf(stderr, "%sOpenGL linker warning: %s\n", pfx.c_str(), log.c_str());
#endif
    } else if (e != GL_TRUE) {
        std::string pfx = (where.length() > 0 ? where + ": " : "");
        std::fprintf(stderr, "%sOpenGL linker error: %s\n", pfx.c_str(), log.c_str());
        std::exit(1);
    }
}

void xglDeleteProgram(GLuint program)
{
    if (glIsProgram(program)) {
        GLint shader_count;
        glGetProgramiv(program, GL_ATTACHED_SHADERS, &shader_count);
        GLuint* shaders = new GLuint[shader_count];
        glGetAttachedShaders(program, shader_count, NULL, shaders);
        for (int i = 0; i < shader_count; i++)
            glDeleteShader(shaders[i]);
        delete[] shaders;
        glDeleteProgram(program);
    }
}

void xglDeletePrograms(GLsizei n, const GLuint* programs)
{
    for (GLsizei i = 0; i < n; i++)
        xglDeleteProgram(programs[i]);
}
