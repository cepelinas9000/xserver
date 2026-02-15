#include <dix-config.h>


#include "hdrext_priv.h"

#include "glamor_priv.h"

#include "bt2020-linear-to-crt-pq-fragment-shader-fp16.glsl.h"
#include "bt2020-linear-to-crt-pq-vertex-shader-fp16.glsl.h"
#include "sdr-fragment-shader-fp16.glsl.h"
#include "sdr-vertex-shader-fp16.glsl.h"

#include "sdr_fragment_utils.glsl.h"

void FreeHDRScreenPrivateData(HDRScreenPrivateData *priv){

  glDeleteBuffers(1,&priv->default_sdr_params_ubo);
  glDeleteProgram(priv->SDR8bit_to_BT2020_linear);
  free(priv);

}

static GLint
glamor_get_uniform(glamor_program               *prog,
                   glamor_program_location      location,
                   const char                   *name)
{
    GLint uniform;
    if (location && (prog->locations & location) == 0)
        return -2;
    uniform = glGetUniformLocation(prog->prog, name);

    return uniform;
}


static inline void fill_glamor_prog_variables(glamor_program *prog){
    prog->matrix_uniform = glamor_get_uniform(prog, glamor_program_location_none, "v_matrix");
    prog->fg_uniform = glamor_get_uniform(prog, glamor_program_location_fg, "fg");
    prog->bg_uniform = glamor_get_uniform(prog, glamor_program_location_bg, "bg");
    prog->fill_offset_uniform = glamor_get_uniform(prog, glamor_program_location_fillpos, "fill_offset");
    prog->fill_size_inv_uniform = glamor_get_uniform(prog, glamor_program_location_fillpos, "fill_size_inv");
    prog->font_uniform = glamor_get_uniform(prog, glamor_program_location_font, "font");
    prog->bitplane_uniform = glamor_get_uniform(prog, glamor_program_location_bitplane, "bitplane");
    prog->bitmul_uniform = glamor_get_uniform(prog, glamor_program_location_bitplane, "bitmul");
    prog->dash_uniform = glamor_get_uniform(prog, glamor_program_location_dash, "dash");
    prog->dash_length_uniform = glamor_get_uniform(prog, glamor_program_location_dash, "dash_length");
    prog->atlas_uniform = glamor_get_uniform(prog, glamor_program_location_atlas, "atlas");

}


void InitializeScreenShaders(ScreenPtr pScreen){



  glamor_screen_private *glamor_priv = glamor_get_screen_private(pScreen);
  glamor_make_current(glamor_priv);


  HDRScreenPrivateData *priv = dixLookupPrivate(&pScreen->devPrivates, HdrScrPrivateKey);
  if (priv){
      FreeHDRScreenPrivateData(priv);
  }


  priv = calloc(1,sizeof(HDRScreenPrivateData));
  dixSetPrivate(&pScreen->devPrivates,HdrScrPrivateKey,priv);

  /* upload includes */

  glNamedStringARB(GL_SHADER_INCLUDE_ARB,-1,"/sdr_fragment_utils.glsl",-1,(const char *)sdr_fragment_utils_glsl);

  /* on error - it will be fatal later with obvious error */


  /* initialize defaults */
  HDR_SDRPARAMS_uniform_t_init(&priv->default_sdr_params);

  glGenBuffers(1, &priv->default_sdr_params_ubo);
  glBindBuffer(GL_UNIFORM_BUFFER, priv->default_sdr_params_ubo);
  glBufferData(GL_UNIFORM_BUFFER, sizeof(priv->default_sdr_params), &priv->default_sdr_params, GL_STATIC_DRAW);


    if (!glamor_priv->copy_area_prog.prog) {
         glamor_build_program(pScreen, &glamor_priv->copy_area_prog,
                                  &glamor_facet_copyarea, NULL, NULL, NULL);
    }

  /* SDR to BT2020 linear */

  {
    GLuint fs = glamor_compile_glsl_prog(GL_FRAGMENT_SHADER,(char *)sdr_fragment_shader_fp16_glsl);
    GLuint vs= glamor_compile_glsl_prog(GL_VERTEX_SHADER,(char *)sdr_vertex_shader_fp16_glsl);

    priv->SDR8bit_to_BT2020_linear = glCreateProgram();
    glAttachShader(priv->SDR8bit_to_BT2020_linear, vs);
    glAttachShader(priv->SDR8bit_to_BT2020_linear, fs);
    glDeleteShader(vs);
    glDeleteShader(fs);

    glamor_link_glsl_prog(pScreen,priv->SDR8bit_to_BT2020_linear,"SDR8bit_to_BT2020_linear");

    glamor_program *prog =  &priv->SDR8bit_to_BT2020_linear_glamor;


    prog->locations = glamor_priv->copy_area_prog.locations;
    prog->prim_use = glamor_priv->copy_area_prog.prim_use;
    prog->prog = priv->SDR8bit_to_BT2020_linear;
    fill_glamor_prog_variables(prog);



  }

  /* BT2020 linear to monitor colorspace with PQ transfer functions*/
  {
    GLuint fs = glamor_compile_glsl_prog(GL_FRAGMENT_SHADER,(char *)bt2020_linear_to_crt_pq_fragment_shader_fp16_glsl);
    GLuint vs= glamor_compile_glsl_prog(GL_VERTEX_SHADER,(char *)bt2020_linear_to_crt_pq_vertex_shader_fp16_glsl);

    priv->BT2020_linear_to_crt_PQ = glCreateProgram();
    glAttachShader(priv->BT2020_linear_to_crt_PQ, vs);
    glAttachShader(priv->BT2020_linear_to_crt_PQ, fs);
    glDeleteShader(vs);
    glDeleteShader(fs);

    glamor_link_glsl_prog(pScreen,priv->BT2020_linear_to_crt_PQ,"BT2020_linear_to_crt_PQ");

    glamor_program *prog =  &priv->BT2020_linear_to_crt_PQ_glamor;

    prog->locations = glamor_priv->copy_area_prog.locations;
    prog->prim_use = glamor_priv->copy_area_prog.prim_use;
    prog->prog = priv->BT2020_linear_to_crt_PQ;
    fill_glamor_prog_variables(prog);


  }

}

void HDR_copy_find_hdr_glamor_program_set_parameters(ScreenPtr screen,glamor_screen_private *glamor_priv, DrawablePtr src,PixmapPtr src_backing,DrawablePtr dst,PixmapPtr dst_backing,glamor_program *prog){
  HDRPixmapPrivate *src_priv = dixLookupPrivate(&src_backing->devPrivates, HdrPixPrivateKey);
  HDRPixmapPrivate *dst_priv = dixLookupPrivate(&dst_backing->devPrivates, HdrPixPrivateKey);

  HDRScreenPrivateData *hdrscrpriv = dixLookupPrivate(&screen->devPrivates, HdrScrPrivateKey);

  /* assume as ordinaly copy */
  if (src_priv->purpose == HDR_pixmap_SDR_OR_PASSTHROUGH && dst_priv->purpose == HDR_pixmap_SDR_OR_PASSTHROUGH){
    return;
  }
  if (src_priv->purpose == HDR_pixmap_intermiate && dst_priv->purpose == HDR_pixmap_crt){

        if (hdrscrpriv->CRT_color_matrices[dst_priv->crt_num] == 0){
            glBindBufferBase(GL_UNIFORM_BUFFER ,3, hdrscrpriv->CRT_color_matrices[0]);
        } else {
            glBindBufferBase(GL_UNIFORM_BUFFER ,3, hdrscrpriv->CRT_color_matrices[dst_priv->crt_num]);
        }



  }

  if (src_priv->purpose == HDR_pixmap_SDR_OR_PASSTHROUGH && dst_priv->purpose == HDR_pixmap_intermiate){
    /* anything with 15,16, 24,32 bits assume SDR */
    if (src_backing->drawable.depth == 15 || src_backing->drawable.depth == 16 ||  src_backing->drawable.depth == 24 || src_backing->drawable.depth == 32){ /* assume sdr -> hdr */

        /* cepelinas9000 nice explanation would be nice as now logically thinking is ok - we are drawing pixmap on window, but in reality bliting it on backing buffer using unrelated pixmap */

        GLuint params_ubo =  hdrscrpriv->default_sdr_params_ubo;
        if (dst->type == DRAWABLE_WINDOW){
          /* check if window have different parameters for sdr */
          WindowPtr res = (WindowPtr)(dst);
          if (res){

              HDRExtWindowPrivate *priv = dixLookupPrivate(&res->devPrivates, HdrWinPrivateKey);
              if (priv->sdr_params_u){
                  params_ubo = priv->sdr_params_u;
              }
          }
        }

        glBindBufferBase(GL_UNIFORM_BUFFER ,3, params_ubo);

    }
  }

}

void HDR_copy_find_hdr_glamor_program(ScreenPtr screen,glamor_screen_private *glamor_priv, DrawablePtr src,PixmapPtr src_backing,DrawablePtr dst,PixmapPtr dst_backing,glamor_program **prog)
{

  HDRPixmapPrivate *src_priv = dixLookupPrivate(&src_backing->devPrivates, HdrPixPrivateKey);
  HDRPixmapPrivate *dst_priv = dixLookupPrivate(&dst_backing->devPrivates, HdrPixPrivateKey);

  HDRScreenPrivateData *hdrpriv = dixLookupPrivate(&screen->devPrivates, HdrScrPrivateKey);


  /* assume as ordinaly copy */
  if (src_priv->purpose == HDR_pixmap_SDR_OR_PASSTHROUGH && dst_priv->purpose == HDR_pixmap_SDR_OR_PASSTHROUGH){
    return;
  }

  if (src_priv->purpose == HDR_pixmap_intermiate && dst_priv->purpose == HDR_pixmap_crt){

        *prog = &hdrpriv->BT2020_linear_to_crt_PQ_glamor;
  }


  if (src_priv->purpose == HDR_pixmap_SDR_OR_PASSTHROUGH && dst_priv->purpose == HDR_pixmap_intermiate){
    if (src_backing->drawable.depth == 15 || src_backing->drawable.depth == 16 ||  src_backing->drawable.depth == 24 || src_backing->drawable.depth == 32){ /* assume sdr -> hdr */
        *prog = &hdrpriv->SDR8bit_to_BT2020_linear_glamor;
    }
  }



}


void hdr_WindowUpdateSDRParams(ScreenPtr screen, WindowPtr pwindow, HDR_SDRPARAMS_uniform_t *sdrparams){

  glamor_screen_private *glamor_priv = glamor_get_screen_private(screen);
  glamor_make_current(glamor_priv);

  HDRExtWindowPrivate *p = dixLookupPrivate(&pwindow->devPrivates, HdrWinPrivateKey);

  /* handle root window */
  if (pwindow == pwindow->drawable.pScreen->root){

    HDRScreenPrivateData *s = dixLookupPrivate(&screen->devPrivates, HdrScrPrivateKey);

    memcpy(&s->default_sdr_params,sdrparams,sizeof(HDR_SDRPARAMS_uniform_t));
    memcpy(&p->sdr_params,sdrparams,sizeof(HDR_SDRPARAMS_uniform_t));

    glBindBuffer(GL_UNIFORM_BUFFER, s->default_sdr_params_ubo);
    glBufferData(GL_UNIFORM_BUFFER, sizeof(HDR_SDRPARAMS_uniform_t), sdrparams, GL_STATIC_DRAW);

    p->sdr_params_u = s->default_sdr_params_ubo;

    return;
  }


  /* handle non root window */

  if (p->sdr_params_u == 0){
    glGenBuffers(1, &p->sdr_params_u);
  }

  memcpy(&p->sdr_params,sdrparams,sizeof(p->sdr_params));

  glBindBuffer(GL_UNIFORM_BUFFER, p->sdr_params_u);
  glBufferData(GL_UNIFORM_BUFFER, sizeof(p->sdr_params), &p->sdr_params, GL_STATIC_DRAW);

}

void hdr_setglprogram_params(ScreenPtr pScreen, glamor_program *prog, DrawablePtr src,DrawablePtr dst,HDR_conversion_e conv){
    switch (conv) {
        case  HDR_conversion_SDR_to_Intermiate:
        {
            HDRScreenPrivateData *hdrscrpriv = dixLookupPrivate(&pScreen->devPrivates, HdrScrPrivateKey);
            GLuint params_ubo =  hdrscrpriv->default_sdr_params_ubo;

            if (src->type == DRAWABLE_WINDOW){
                WindowPtr res = (WindowPtr)(src);
                HDRExtWindowPrivate *priv = dixLookupPrivate(&res->devPrivates, HdrWinPrivateKey);
                if (priv->sdr_params_u){
                     params_ubo = priv->sdr_params_u;
                }
            }
        glBindBufferBase(GL_UNIFORM_BUFFER ,3, params_ubo);
        }
        break;
    default:
        break;
}
}

// !!!! This is one generated!!!!
// Helper: print a single uniform value from a pointer.
// 'count' is always 1 in this simplified version.
#if 0
static void print_uniform_value(const void *data, GLenum type, int count) {
    // count is always 1, but the function still multiplies components by count.
    switch (type) {
        case GL_FLOAT:
        case GL_FLOAT_VEC2:
        case GL_FLOAT_VEC3:
        case GL_FLOAT_VEC4:
        case GL_FLOAT_MAT2:
        case GL_FLOAT_MAT3:
        case GL_FLOAT_MAT4:
        case GL_FLOAT_MAT2x3:
        case GL_FLOAT_MAT2x4:
        case GL_FLOAT_MAT3x2:
        case GL_FLOAT_MAT3x4:
        case GL_FLOAT_MAT4x2:
        case GL_FLOAT_MAT4x3:
            {
                const GLfloat *f = (const GLfloat*)data;
                int components = 1;
                if (type == GL_FLOAT_VEC2) components = 2;
                else if (type == GL_FLOAT_VEC3) components = 3;
                else if (type == GL_FLOAT_VEC4) components = 4;
                else if (type == GL_FLOAT_MAT2) components = 4;
                else if (type == GL_FLOAT_MAT3) components = 9;
                else if (type == GL_FLOAT_MAT4) components = 16;
                else if (type == GL_FLOAT_MAT2x3) components = 6;
                else if (type == GL_FLOAT_MAT2x4) components = 8;
                else if (type == GL_FLOAT_MAT3x2) components = 6;
                else if (type == GL_FLOAT_MAT3x4) components = 12;
                else if (type == GL_FLOAT_MAT4x2) components = 8;
                else if (type == GL_FLOAT_MAT4x3) components = 12;

                int total = components * count;  // count == 1
                printf("[");
                for (int i = 0; i < total; ++i) {
                    if (i > 0) printf(", ");
                    printf("%f", f[i]);
                }
                printf("]");
            }
            break;

        case GL_INT:
        case GL_INT_VEC2:
        case GL_INT_VEC3:
        case GL_INT_VEC4:
            {
                const GLint *i = (const GLint*)data;
                int components = 1;
                if (type == GL_INT_VEC2) components = 2;
                else if (type == GL_INT_VEC3) components = 3;
                else if (type == GL_INT_VEC4) components = 4;
                int total = components * count;
                printf("[");
                for (int j = 0; j < total; ++j) {
                    if (j > 0) printf(", ");
                    printf("%d", i[j]);
                }
                printf("]");
            }
            break;

        case GL_UNSIGNED_INT:
        case GL_UNSIGNED_INT_VEC2:
        case GL_UNSIGNED_INT_VEC3:
        case GL_UNSIGNED_INT_VEC4:
            {
                const GLuint *ui = (const GLuint*)data;
                int components = 1;
                if (type == GL_UNSIGNED_INT_VEC2) components = 2;
                else if (type == GL_UNSIGNED_INT_VEC3) components = 3;
                else if (type == GL_UNSIGNED_INT_VEC4) components = 4;
                int total = components * count;
                printf("[");
                for (int j = 0; j < total; ++j) {
                    if (j > 0) printf(", ");
                    printf("%u", ui[j]);
                }
                printf("]");
            }
            break;

        case GL_DOUBLE:
        case GL_DOUBLE_VEC2:
        case GL_DOUBLE_VEC3:
        case GL_DOUBLE_VEC4:
        case GL_DOUBLE_MAT2:
        case GL_DOUBLE_MAT3:
        case GL_DOUBLE_MAT4:
        case GL_DOUBLE_MAT2x3:
        case GL_DOUBLE_MAT2x4:
        case GL_DOUBLE_MAT3x2:
        case GL_DOUBLE_MAT3x4:
        case GL_DOUBLE_MAT4x2:
        case GL_DOUBLE_MAT4x3:
            {
                const GLdouble *d = (const GLdouble*)data;
                int components = 1;
                if (type == GL_DOUBLE_VEC2) components = 2;
                else if (type == GL_DOUBLE_VEC3) components = 3;
                else if (type == GL_DOUBLE_VEC4) components = 4;
                else if (type == GL_DOUBLE_MAT2) components = 4;
                else if (type == GL_DOUBLE_MAT3) components = 9;
                else if (type == GL_DOUBLE_MAT4) components = 16;
                else if (type == GL_DOUBLE_MAT2x3) components = 6;
                else if (type == GL_DOUBLE_MAT2x4) components = 8;
                else if (type == GL_DOUBLE_MAT3x2) components = 6;
                else if (type == GL_DOUBLE_MAT3x4) components = 12;
                else if (type == GL_DOUBLE_MAT4x2) components = 8;
                else if (type == GL_DOUBLE_MAT4x3) components = 12;
                int total = components * count;
                printf("[");
                for (int j = 0; j < total; ++j) {
                    if (j > 0) printf(", ");
                    printf("%f", d[j]);
                }
                printf("]");
            }
            break;

        case GL_BOOL:
        case GL_BOOL_VEC2:
        case GL_BOOL_VEC3:
        case GL_BOOL_VEC4:
            {
                const GLint *b = (const GLint*)data;
                int components = 1;
                if (type == GL_BOOL_VEC2) components = 2;
                else if (type == GL_BOOL_VEC3) components = 3;
                else if (type == GL_BOOL_VEC4) components = 4;
                int total = components * count;
                printf("[");
                for (int j = 0; j < total; ++j) {
                    if (j > 0) printf(", ");
                    printf("%s", b[j] ? "true" : "false");
                }
                printf("]");
            }
            break;

        default:
            printf("<unhandled type 0x%x>", type);
    }
}

void dump_uniforms(GLuint program) {
    GLint i, j;

    GLint current_prog = 0;
    glGetIntegerv(GL_CURRENT_PROGRAM, &current_prog);
    if ((GLuint)current_prog != program) {
        fprintf(stderr, "Warning: program %u is not currently bound. Results may be incorrect.\n", program);
    }

    GLint numUniforms = 0, numUniformBlocks = 0, maxNameLen = 0;
    glGetProgramiv(program, GL_ACTIVE_UNIFORMS, &numUniforms);
    glGetProgramiv(program, GL_ACTIVE_UNIFORM_BLOCKS, &numUniformBlocks);
    glGetProgramiv(program, GL_ACTIVE_UNIFORM_MAX_LENGTH, &maxNameLen);

    if (maxNameLen == 0) maxNameLen = 256;
    char *name = (char*)malloc(maxNameLen);

    printf("=== Dumping uniforms for program %u ===\n", program);

    // ------------------------------------------------------------------
    // 1. Regular uniforms (not in a named uniform block)
    // ------------------------------------------------------------------
    printf("\n--- Regular Uniforms ---\n");
    for (i = 0; i < numUniforms; ++i) {
        GLsizei length;
        GLint size;
        GLenum type;
        glGetActiveUniform(program, i, maxNameLen, &length, &size, &type, name);

        GLint blockIndex = -1;
        glGetActiveUniformsiv(program, 1, (GLuint*)&i, GL_UNIFORM_BLOCK_INDEX, &blockIndex);
        if (blockIndex != -1) continue;   // part of a block, handle later

        GLint loc = glGetUniformLocation(program, name);
        if (loc == -1) {
            printf("  %s: <location -1, skipped>\n", name);
            continue;
        }

        // Print name, type, and size (size may be >1 for arrays, but we only show the first element)
        printf("  %s (type 0x%x, size %d) = ", name, type, size);
        if (size > 1) {
            printf("(array of %d elements, showing first element only) ", size);
        }

        // Retrieve the value (first element if array)
        switch (type) {
            case GL_FLOAT:
            case GL_FLOAT_VEC2:
            case GL_FLOAT_VEC3:
            case GL_FLOAT_VEC4:
            case GL_FLOAT_MAT2:
            case GL_FLOAT_MAT3:
            case GL_FLOAT_MAT4:
            case GL_FLOAT_MAT2x3:
            case GL_FLOAT_MAT2x4:
            case GL_FLOAT_MAT3x2:
            case GL_FLOAT_MAT3x4:
            case GL_FLOAT_MAT4x2:
            case GL_FLOAT_MAT4x3:
                {
                    int comps = 1;
                    if (type == GL_FLOAT_VEC2) comps = 2;
                    else if (type == GL_FLOAT_VEC3) comps = 3;
                    else if (type == GL_FLOAT_VEC4) comps = 4;
                    else if (type == GL_FLOAT_MAT2) comps = 4;
                    else if (type == GL_FLOAT_MAT3) comps = 9;
                    else if (type == GL_FLOAT_MAT4) comps = 16;
                    else if (type == GL_FLOAT_MAT2x3) comps = 6;
                    else if (type == GL_FLOAT_MAT2x4) comps = 8;
                    else if (type == GL_FLOAT_MAT3x2) comps = 6;
                    else if (type == GL_FLOAT_MAT3x4) comps = 12;
                    else if (type == GL_FLOAT_MAT4x2) comps = 8;
                    else if (type == GL_FLOAT_MAT4x3) comps = 12;
                    GLfloat val[16];
                    glGetUniformfv(program, loc, val);
                    print_uniform_value(val, type, 1);
                }
                break;
            case GL_INT:
            case GL_INT_VEC2:
            case GL_INT_VEC3:
            case GL_INT_VEC4:
                {
                    int comps = 1;
                    if (type == GL_INT_VEC2) comps = 2;
                    else if (type == GL_INT_VEC3) comps = 3;
                    else if (type == GL_INT_VEC4) comps = 4;
                    GLint val[4];
                    glGetUniformiv(program, loc, val);
                    print_uniform_value(val, type, 1);
                }
                break;
            case GL_UNSIGNED_INT:
            case GL_UNSIGNED_INT_VEC2:
            case GL_UNSIGNED_INT_VEC3:
            case GL_UNSIGNED_INT_VEC4:
                {
                    int comps = 1;
                    if (type == GL_UNSIGNED_INT_VEC2) comps = 2;
                    else if (type == GL_UNSIGNED_INT_VEC3) comps = 3;
                    else if (type == GL_UNSIGNED_INT_VEC4) comps = 4;
                    GLuint val[4];
                    glGetUniformuiv(program, loc, val);
                    print_uniform_value(val, type, 1);
                }
                break;
            case GL_DOUBLE:
            case GL_DOUBLE_VEC2:
            case GL_DOUBLE_VEC3:
            case GL_DOUBLE_VEC4:
            case GL_DOUBLE_MAT2:
            case GL_DOUBLE_MAT3:
            case GL_DOUBLE_MAT4:
            case GL_DOUBLE_MAT2x3:
            case GL_DOUBLE_MAT2x4:
            case GL_DOUBLE_MAT3x2:
            case GL_DOUBLE_MAT3x4:
            case GL_DOUBLE_MAT4x2:
            case GL_DOUBLE_MAT4x3:
                {
                    int comps = 1;
                    if (type == GL_DOUBLE_VEC2) comps = 2;
                    else if (type == GL_DOUBLE_VEC3) comps = 3;
                    else if (type == GL_DOUBLE_VEC4) comps = 4;
                    else if (type == GL_DOUBLE_MAT2) comps = 4;
                    else if (type == GL_DOUBLE_MAT3) comps = 9;
                    else if (type == GL_DOUBLE_MAT4) comps = 16;
                    else if (type == GL_DOUBLE_MAT2x3) comps = 6;
                    else if (type == GL_DOUBLE_MAT2x4) comps = 8;
                    else if (type == GL_DOUBLE_MAT3x2) comps = 6;
                    else if (type == GL_DOUBLE_MAT3x4) comps = 12;
                    else if (type == GL_DOUBLE_MAT4x2) comps = 8;
                    else if (type == GL_DOUBLE_MAT4x3) comps = 12;
                    GLdouble val[16];
                    glGetUniformdv(program, loc, val);
                    print_uniform_value(val, type, 1);
                }
                break;
            case GL_BOOL:
            case GL_BOOL_VEC2:
            case GL_BOOL_VEC3:
            case GL_BOOL_VEC4:
                {
                    int comps = 1;
                    if (type == GL_BOOL_VEC2) comps = 2;
                    else if (type == GL_BOOL_VEC3) comps = 3;
                    else if (type == GL_BOOL_VEC4) comps = 4;
                    GLint val[4];
                    glGetUniformiv(program, loc, val);
                    print_uniform_value(val, type, 1);
                }
                break;
            default:
                printf("<unhandled type 0x%x>", type);
        }
        printf("\n");
    }

    // ------------------------------------------------------------------
    // 2. Uniform blocks
    // ------------------------------------------------------------------
    if (numUniformBlocks > 0) {
        printf("\n--- Uniform Blocks ---\n");
    }

    for (i = 0; i < numUniformBlocks; ++i) {
        glGetActiveUniformBlockName(program, i, maxNameLen, NULL, name);
        printf("\nBlock %d: %s\n", i, name);

        GLint blockSize, binding;
        glGetActiveUniformBlockiv(program, i, GL_UNIFORM_BLOCK_DATA_SIZE, &blockSize);
        glGetActiveUniformBlockiv(program, i, GL_UNIFORM_BLOCK_BINDING, &binding);

        GLint numBlockUniforms;
        glGetActiveUniformBlockiv(program, i, GL_UNIFORM_BLOCK_ACTIVE_UNIFORMS, &numBlockUniforms);
        GLint *uniformIndices = (GLint*)malloc(numBlockUniforms * sizeof(GLint));
        glGetActiveUniformBlockiv(program, i, GL_UNIFORM_BLOCK_ACTIVE_UNIFORM_INDICES, uniformIndices);

        GLint bufferId = 0;
        glGetIntegeri_v(GL_UNIFORM_BUFFER_BINDING, binding, &bufferId);
        if (bufferId == 0) {
            printf("  No buffer bound to binding point %d\n", binding);
            free(uniformIndices);
            continue;
        }

        GLubyte *blockData = (GLubyte*)malloc(blockSize);
        glBindBuffer(GL_UNIFORM_BUFFER, bufferId);
        glGetBufferSubData(GL_UNIFORM_BUFFER, 0, blockSize, blockData);
        glBindBuffer(GL_UNIFORM_BUFFER, 0);

        for (j = 0; j < numBlockUniforms; ++j) {
            GLuint uIndex = uniformIndices[j];

            GLsizei length;
            GLint size;
            GLenum type;
            glGetActiveUniform(program, uIndex, maxNameLen, &length, &size, &type, name);

            GLint offset, arrayStride, matrixStride;
            glGetActiveUniformsiv(program, 1, &uIndex, GL_UNIFORM_OFFSET, &offset);
            glGetActiveUniformsiv(program, 1, &uIndex, GL_UNIFORM_ARRAY_STRIDE, &arrayStride);
            glGetActiveUniformsiv(program, 1, &uIndex, GL_UNIFORM_MATRIX_STRIDE, &matrixStride);

            printf("  %s (offset %d, arrayStride %d, matrixStride %d, type 0x%x, size %d) = ",
                   name, offset, arrayStride, matrixStride, type, size);

            if (size > 1) {
                printf("(array of %d elements, showing first element only) ", size);
            }

            // Pointer to the first element (if array) or the only element
            char *elemPtr = (char*)blockData + offset;

            // Check if it's a matrix type
            int isMatrix = 0;
            int rows = 0, cols = 0;
            switch (type) {
                case GL_FLOAT_MAT2:   isMatrix=1; rows=2; cols=2; break;
                case GL_FLOAT_MAT3:   isMatrix=1; rows=3; cols=3; break;
                case GL_FLOAT_MAT4:   isMatrix=1; rows=4; cols=4; break;
                case GL_FLOAT_MAT2x3: isMatrix=1; rows=3; cols=2; break;
                case GL_FLOAT_MAT2x4: isMatrix=1; rows=4; cols=2; break;
                case GL_FLOAT_MAT3x2: isMatrix=1; rows=2; cols=3; break;
                case GL_FLOAT_MAT3x4: isMatrix=1; rows=4; cols=3; break;
                case GL_FLOAT_MAT4x2: isMatrix=1; rows=2; cols=4; break;
                case GL_FLOAT_MAT4x3: isMatrix=1; rows=3; cols=4; break;
                case GL_DOUBLE_MAT2:   isMatrix=1; rows=2; cols=2; break;
                case GL_DOUBLE_MAT3:   isMatrix=1; rows=3; cols=3; break;
                case GL_DOUBLE_MAT4:   isMatrix=1; rows=4; cols=4; break;
                case GL_DOUBLE_MAT2x3: isMatrix=1; rows=3; cols=2; break;
                case GL_DOUBLE_MAT2x4: isMatrix=1; rows=4; cols=2; break;
                case GL_DOUBLE_MAT3x2: isMatrix=1; rows=2; cols=3; break;
                case GL_DOUBLE_MAT3x4: isMatrix=1; rows=4; cols=3; break;
                case GL_DOUBLE_MAT4x2: isMatrix=1; rows=2; cols=4; break;
                case GL_DOUBLE_MAT4x3: isMatrix=1; rows=3; cols=4; break;
            }

            if (isMatrix) {
                printf("\n      ");
                for (int col = 0; col < cols; ++col) {
                    char *colPtr = elemPtr + col * matrixStride;
                    if (type >= GL_FLOAT_MAT2 && type <= GL_FLOAT_MAT4x3) {
                        GLfloat *fcol = (GLfloat*)colPtr;
                        printf("col%d: [", col);
                        for (int r = 0; r < rows; ++r) {
                            if (r > 0) printf(", ");
                            printf("%f", fcol[r]);
                        }
                        printf("] ");
                    } else if (type >= GL_DOUBLE_MAT2 && type <= GL_DOUBLE_MAT4x3) {
                        GLdouble *dcol = (GLdouble*)colPtr;
                        printf("col%d: [", col);
                        for (int r = 0; r < rows; ++r) {
                            if (r > 0) printf(", ");
                            printf("%f", dcol[r]);
                        }
                        printf("] ");
                    }
                }
            } else {
                print_uniform_value(elemPtr, type, 1);
            }
            printf("\n");
        }

        free(uniformIndices);
        free(blockData);
    }

    free(name);
    printf("\n=== End dump ===\n");
}
#endif
