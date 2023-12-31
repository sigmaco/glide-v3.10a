/*
** THIS SOFTWARE IS SUBJECT TO COPYRIGHT PROTECTION AND IS OFFERED ONLY
** PURSUANT TO THE 3DFX GLIDE GENERAL PUBLIC LICENSE. THERE IS NO RIGHT
** TO USE THE GLIDE TRADEMARK WITHOUT PRIOR WRITTEN PERMISSION OF 3DFX
** INTERACTIVE, INC. A COPY OF THIS LICENSE MAY BE OBTAINED FROM THE 
** DISTRIBUTOR OR BY CONTACTING 3DFX INTERACTIVE INC(info@3dfx.com). 
** THIS PROGRAM IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY KIND, EITHER 
** EXPRESSED OR IMPLIED. SEE THE 3DFX GLIDE GENERAL PUBLIC LICENSE FOR A
** FULL TEXT OF THE NON-WARRANTY PROVISIONS.  
** 
** USE, DUPLICATION OR DISCLOSURE BY THE GOVERNMENT IS SUBJECT TO
** RESTRICTIONS AS SET FORTH IN SUBDIVISION (C)(1)(II) OF THE RIGHTS IN
** TECHNICAL DATA AND COMPUTER SOFTWARE CLAUSE AT DFARS 252.227-7013,
** AND/OR IN SIMILAR OR SUCCESSOR CLAUSES IN THE FAR, DOD OR NASA FAR
** SUPPLEMENT. UNPUBLISHED RIGHTS RESERVED UNDER THE COPYRIGHT LAWS OF
** THE UNITED STATES.  
** 
** COPYRIGHT 3DFX INTERACTIVE, INC. 1999, ALL RIGHTS RESERVED
**
** $Revision: 1.2 $
** $Date: 2000/06/15 00:11:40 $
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "texusint.h"

#ifdef GLIDE3
#define MAX_TEX_DIM   2048
#define MAX_TEX_LEVEL   12
#else
#define MAX_TEX_DIM    256
#define MAX_TEX_LEVEL    9
#endif

typedef struct {
  int   resize_en, resize_auto, resize_width, resize_height; 
  int   mipmap_en, mipmap_auto, mipmap_lod;
  int   dither, compression, pixel_format; 
  int   view_en, view_background;
  int   alpha_en;     // force input alphas to 0xff
  // some tga32 files (pluto) don't have alphas

  int   nfiles;       // these two work like argc, argv.
  char  **files;                
  int   output_en, output_split;
  char  output_base[128], output_ext[128];
} TxOpts;


/* This stuff is strictly for de-globifying cmdline args */
#if defined __WATCOMC__
#include <direct.h>
#elif defined _WIN32
#include <direct.h>
#include <fxglob.h>
#else
#include <dirent.h>
#endif

static  char    *progname;
static  char    *usage_string[] = {
  " TEXUS (Texus 2.01) (c) 3dfx Interactive Inc.\n",
  "\n",
  " Usage : Texus [options] <input-files>\n",
  // " Input : -is            Assume input files make a single mipmap\n",
  "\n",
  " Resize: -r           * Auto-Resample input image to smaller power of 2\n",
  "         -rn            Don't Auto-Resample input image.\n",
  "         -r<aspect>     Resample to <aspect>={8x1|4x1|2x1|1x1|1x2|1x4|1x8}\n",
  "         -R w h         Resample input image to absolute width, height\n",
  " Mipmap: -m           * Generate all levels of mipmaps\n",
  "         -mn            Don't generate mipmaps\n",
#ifdef GLIDE3
  "         -m[1-12]       Generate exactly n levels of mipmaps, n=1-12\n",
  "         -M size        Smallest mipmap is <= size (1 <= size <= 2048)\n",
#else
  "         -m[1-9]        Generate exactly n levels of mipmaps, n=1-9\n",
  "         -M size        Smallest mipmap is <= size (1 <= size <= 256)\n",
#endif
  " Dither: -de          * Dithering - error diffusion\n",
  "         -d4            Dithering - 4x4 ordered dither\n",
  "         -dn            Don't dither\n"
  " Format: -t <format>    Texel Format {argb8888(*)|rgb332|yiq|a8|i8|ai44|p8|\n",
  "                        argb8332|ayiq|rgb565|argb1555|argb4444|ai88|ap88}\n",
  "         -y[sh]         YIQ compression: s = statistical, h = heuristic (*)\n"
  " Output: -o <file>.ext  Generate output to <file>.ext, .ext={.3df(*)|.tga}\n",
  "                        If <file> is NULL, basename of input file is used\n",
  NULL
};

static void 
usage(char *format, int value)
{
  char    **u = usage_string;

  while (*u) {
    fputs(*u, stderr);
    u++;
  }
        
  if (format) {
    fprintf(stderr, format, value);
  }
  exit(0);
}

static void
_txParseCmdline(int argc, char **argv, TxOpts *txOptions)
{

  progname = *argv;

  /* Set initial options */
  txOptions->resize_en            = FXTRUE; 
  txOptions->resize_auto          = FXTRUE; 
  txOptions->resize_width         = -1; 
  txOptions->resize_height        = -1;

  txOptions->mipmap_en            = FXTRUE; 
  txOptions->mipmap_auto          = FXTRUE; 
  txOptions->mipmap_lod           = -1; 

  txOptions->output_en            = FXFALSE; 
  txOptions->output_split         = FXFALSE; 
  txOptions->output_base[0]       = 0;
  txOptions->output_ext[0]        = 0;

  txOptions->dither               = TX_DITHER_ERR;
  txOptions->compression          = TX_COMPRESSION_HEURISTIC;

  txOptions->pixel_format         = GR_TEXFMT_RGB_565;
  txOptions->view_en              = FXFALSE;
  txOptions->view_background      = 0;
        
  txOptions->alpha_en             = FXFALSE;

  /* parse command line options */
  while (--argc > 0 && **++argv == '-') {
    char    *token;
    int             a, b;
    char    *c;

    token = argv[0] + 1;

    switch(*token) {

    case 'a':       txOptions->alpha_en = 1;
      if (*++token == 't') {
        // alpha - internal test mode.
        txOptions->alpha_en = 2;
        printf("Atest mode\n");
      }
      break;

    case 'r':               // -r  resample, snapping to lower power of 2
      c = ++token;
      if (*c == 0) {                  // -r is default option.
      }
      if (*c == 'n')  {               // -rn, don't resample automatically
        txOptions->resize_en     = FXFALSE;
        txOptions->resize_auto   = FXFALSE;
        txOptions->resize_width  = -1;
        txOptions->resize_height = -1;
      } else {
        if (*token) {
          if          (0);
          // 8 bit formats
          else    if (!strcmp(c, "8x1")) 
            txOptions->resize_width  = 8, 
              txOptions->resize_height = 1;
          else    if (!strcmp(c, "4x1")) 
            txOptions->resize_width  = 4, 
              txOptions->resize_height = 1;
          else    if (!strcmp(c, "2x1")) 
            txOptions->resize_width  = 2, 
              txOptions->resize_height = 1;
          else    if (!strcmp(c, "1x1")) 
            txOptions->resize_width  = 1, 
              txOptions->resize_height = 1;
          else    if (!strcmp(c, "1x2")) 
            txOptions->resize_width  = 1, 
              txOptions->resize_height = 2;
          else    if (!strcmp(c, "1x4")) 
            txOptions->resize_width  = 1, 
              txOptions->resize_height = 4;
          else    if (!strcmp(c, "1x8")) 
            txOptions->resize_width  = 1, 
              txOptions->resize_height = 8;
          else    
            usage("***Bad option: -r{|n|<aspect>}\n", 0);
        }
      }
      break;


    case 'R':               // -R <width> <height>  resample to specific size
      if (argc < 3) usage("***Bad option: -R <width> <height>\n", 0);
      a = atoi(*++argv);
      b = atoi(*++argv);
      if ((a >= 1) && (b >= 1)) {
        txOptions->resize_en = FXTRUE;
        txOptions->resize_auto = FXFALSE;
        txOptions->resize_width = a;
        txOptions->resize_height = b;
      } else {
        usage("***Bad option: -R <bad_w> <bad_h>\n", 0);
      }
      argc -= 2;
      break;

    case 'm':               
      switch (*++token) {
      case  0 :
        break;                  // -m is default.

      case 'n':                               // -mn, disable mipmaps.
        txOptions->mipmap_en   = FXFALSE;
        txOptions->mipmap_auto = FXFALSE;
        break;
#ifdef GLIDE3
      case '1': /* Handle cases for "1", "10", "11", and "12" */
        if (*(token+1) == 0) {
          /* -m1 */
          txOptions->mipmap_en   = FXTRUE;
          txOptions->mipmap_auto = FXTRUE;
          txOptions->mipmap_lod  = *token - '0';
        } else {
          /* What's the next digit */
          switch (*(token+1)) {
          case '0':
          case '1':
          case '2':
            txOptions->mipmap_en   = FXTRUE;
            txOptions->mipmap_auto = FXTRUE;
            txOptions->mipmap_lod  = 10 + *(token+1) - '0';
            break;
          default:
            usage("***Bad option -m%c\n", *token);
            break;
          }
        }
        break;

      case '2': case '3': case '4': case '5':
#else
      case '1': case '2': case '3': case '4': case '5':
#endif
      case '6': case '7': case '8': case '9': 
        if (*(token+1) != 0) {
          usage("***Bad option -m\n", 0);
        }
        txOptions->mipmap_en   = FXTRUE;
        txOptions->mipmap_auto = FXTRUE;
        txOptions->mipmap_lod  = *token - '0';
        break;

      default:
        usage("***Bad option -m%c\n", *token);
        break;
      }
      break;

    case 'M':               // -M <smallest_dim>
      if (argc < 2) usage("***Bad option: -M <size>\n", 0);
      a = atoi(*++argv);
      argc --;
      if (a >= 1) {
        txOptions->mipmap_en    = FXTRUE;
        txOptions->mipmap_auto  = FXFALSE;
        txOptions->mipmap_lod   = a;
      } else {
        usage("***Bad option: -M <size>\n", 0);
      }
      break;


    case 'd':               // dithering: could be -dn, -d4,
      switch (*++token) {
      case 'e':
        txOptions->dither = TX_DITHER_ERR;
        break;                                  // No dithering is default.
      case 'n':
        txOptions->dither = TX_DITHER_NONE;
        break;

      case '4':
        txOptions->dither = TX_DITHER_4x4;
        break;

      default:
        usage("***Bad option -d%c\n", *token);
        break;
      }
      break;

    case 'y':                               // compression algorithm -yy, -ys, -yn
      switch (*++token) {
#if 0
        // disabled, because it sucks.
      case 'y': txOptions->compression = TX_COMPRESSION_YIQ; break;
#endif
      case 's': txOptions->compression = TX_COMPRESSION_STATISTICAL; break;
      case 'h': txOptions->compression = TX_COMPRESSION_HEURISTIC; break;

      default:
        usage("***Bad option -y%c\n", *token);
        break;
      }
      break;

    case 't':                               // -t, pixel format
      if (argc < 2) usage("***Bad option: -t <format>\n", 0);
      c = *++argv;
      argc--;

                                // printf("Output format = %s\n", c);

      if          (0);

                                // 8 bit formats
      else    if (!strcmp(c, "rgb332")) 
        txOptions->pixel_format = GR_TEXFMT_RGB_332;
      else    if (!strcmp(c, "yiq")) 
        txOptions->pixel_format = GR_TEXFMT_YIQ_422;
      else    if (!strcmp(c, "a8")) 
        txOptions->pixel_format = GR_TEXFMT_A_8;
      else    if (!strcmp(c, "i8")) 
        txOptions->pixel_format = GR_TEXFMT_I_8;
      else    if (!strcmp(c, "ai44")) 
        txOptions->pixel_format = GR_TEXFMT_AI_44;
      else    if (!strcmp(c, "p8")) 
        txOptions->pixel_format = GR_TEXFMT_P_8;

                                // 16 bit formats
      else    if (!strcmp(c, "argb8332")) 
        txOptions->pixel_format = GR_TEXFMT_ARGB_8332;
      else    if (!strcmp(c, "ayiq")) 
        txOptions->pixel_format = GR_TEXFMT_AYIQ_8422;
      else    if (!strcmp(c, "rgb565")) 
        txOptions->pixel_format = GR_TEXFMT_RGB_565;
      else    if (!strcmp(c, "argb1555")) 
        txOptions->pixel_format = GR_TEXFMT_ARGB_1555;
      else    if (!strcmp(c, "argb4444")) 
        txOptions->pixel_format = GR_TEXFMT_ARGB_4444;
      else    if (!strcmp(c, "ai88")) 
        txOptions->pixel_format = GR_TEXFMT_AI_88;
      else    if (!strcmp(c, "ap88")) 
        txOptions->pixel_format = GR_TEXFMT_AP_88;

                                // 32 bit formats
      else    if (!strcmp(c, "argb8888")) 
        txOptions->pixel_format = GR_TEXFMT_ARGB_8888;

                                // Sorry
      else    usage("*Bad option: -t <format>", 0);
      break;

                        
    case 'o':               // -o, -os <filename>.<ext>
      switch (*++token) {
      case 's':                               
      case 0:                                 
        if (argc < 2) usage("***Bad option: -o <file>\n", 0);
        c = *++argv;
        argc--;
        txOptions->output_en = FXTRUE;
        if (*token == 's') {
          txOptions->output_split = FXTRUE;
        } else {
          txOptions->output_split = FXFALSE;
        }
        txPathAndBasename(c, txOptions->output_base);
        if (strcmp(txOptions->output_base, "*") == 0) {
          txOptions->output_base[0] = 0;
        }
        txExtension(c, txOptions->output_ext);
        if (strcmp(txOptions->output_ext, ".3df") &&
            strcmp(txOptions->output_ext, ".tga")) {
          usage("*Bad option: -o%c <file>{.3df|.tga}\n", 
                *token);
        }
        // printf("-o %s\n", c);
        break;

      default:
        usage("***Bad option -o%c\n", *token);
        break;
      }
      break;

    case 'v':               // -vn, -v1, -v0, -v
      switch (*++token) {
      case 'n':                               
        txOptions->view_en = FXFALSE;
        break;

      case '1':                                 
        txOptions->view_background = 1;
        break;

      case '0':
        txOptions->view_background = 0;
        break;

      case 0  :
        txOptions->view_en = FXTRUE;
        break;

      default:
        usage("***Bad option -v%c\n", *token);
        break;
      }
      break;


    case '?':               // Just the help screen.
      usage("",0);
      break;

    default:
      usage("***Bad option:  -%c\n", *token);
      break;
    }
  }

#ifdef _WIN32
  fxGlobify( &argc, &argv );
#endif

  txOptions->nfiles = argc;
  txOptions->files = argv;

#if 1
  while (argc-- > 0) {
    // printf("Rest: %s\n", *argv++);
  }
#endif
}
void main(int argc, char **argv)
{
  char    *progname;
  TxOpts  txOptions;
  int             nfiles;
  char    **files;

  txVerbose = 1;
  progname = *argv;
  _txParseCmdline(argc, argv, &txOptions);
  nfiles  = txOptions.nfiles;
  files = txOptions.files;

  if (nfiles == 0) usage("", 0);
  /*
   * If many input files, but only one output file, bad!
   */
  if ((nfiles > 1) &&
      txOptions.output_en &&
      txOptions.output_base[0]) {

    fprintf(stderr, "Texus: many input files, but only one output file specified!\n");
    exit(2);
  }


  while (nfiles--) {
    char    *ifile;
    TxMip   txMip;          // always ARGB8888
    TxMip   pxMip;          // always pixel format specified on cmdline
    int             i, w, h;

    ifile = *files++;

    /*********** READ *************************/
    printf("\n");
    txMipRead(&txMip, ifile, GR_TEXFMT_ARGB_8888);
    if (txMip.data[0] == NULL)
      txPanic("No input image\n");

    if (txOptions.alpha_en) {
      // Force all input alphas to 0xff.
      FxU32   *data32;
      FxU32   a, aincr;

      w = txMip.width;
      h = txMip.height;
                        
      for (i=0; i<txMip.depth; i++) {
        int row, col;
                                
        data32 = (FxU32 *) txMip.data[i];
        col = h;

        while (col--) {
          a = 0xFF000000;
          aincr = (txOptions.alpha_en == 2) ? 0x2000000 : 0;

          for (row = 0; row < w; row++) {
            *data32 &= 0x00FFFFFF;
            *data32 |= a;
            data32++;
            a += aincr;
          }
        }
                                
        if (w > 1) w >>= 1;
        if (h > 1) h >>= 1;
      }
    }
                
    if (txOptions.view_en) {
      printf("Viewing Original Image.\n");
      txMipView(&txMip, ifile, 0, txOptions.view_background);
    }

    /*********** RESAMPLE ********************/
    /*
     *  -r          :resize_en = 1, resize_auto = 1, resize_width = -1.
     *      -r<ar>  :resize_en = 1, resize_auto = 1, resize_width != -1.
     *  -R w h      :resize_en = 1, resize_auto = 0, resize_width,ht = w, h
     */
    if (txOptions.resize_en) {
      int             width, height;
      TxMip           tmpMip;

      if (txOptions.resize_auto) {

        w = txFloorPow2(txMip.width);
        h = txFloorPow2(txMip.height);
        if (w > MAX_TEX_DIM) w = MAX_TEX_DIM;
        if (h > MAX_TEX_DIM) h = MAX_TEX_DIM;

                                /* If an explicit aspect was given, honor it */
        if (txOptions.resize_width != -1) {
          /* 
           * Keep magnifying till we overflow one of the 
           * dimensions.
           */
          width  = txOptions.resize_width;
          height = txOptions.resize_height;

          while ((width  < w) && (height < h)) {
            width  <<= 1;
            height <<= 1;
          }
        } else {
          /*
           * We snapped to the lower power of 2.
           * Fix aspect ratio if necessary.
           */
          if (w >= h) 
            while (w/h > 8) w >>= 1;
          else 
            while (h/w > 8) h >>= 1;
          width  = w;
          height = h;
        }
      } else {
                                /* Explicit resizing info given by user */
        width  = txOptions.resize_width;
        height = txOptions.resize_height;
      }


      tmpMip = txMip;

      tmpMip.width = width;        
      tmpMip.height = height;        
      txMipAlloc( &tmpMip );
                        
      /* 
       * Resample Level 0.
       * Throw away all other levels
       * This is done while resampling, nothing to do here.
       */
      txMipResample(&tmpMip, &txMip );
      txFree( txMip.data[0] );
      txMip = tmpMip;
    } else {
      printf("No Resampling\n");
    }

    /*************** MIPMAPS ************/
    // printf("Mipmaps\n");
    if ((txOptions.mipmap_en == FXFALSE) ||
        (txMip.width & (txMip.width-1))  ||
        (txMip.height & (txMip.height-1)) ) {

      /*
       * Not mipmapped, or not power of 2 on a side.
       */
      if (txOptions.mipmap_en) {
        fprintf(stderr, "Warning: Mipmapping disabled, not power of 2\n");
      }

      /*
       * Use less memory.
       */
      txMip.depth = 1;
      txMip.size  = txMip.width * txMip.height * 4;
      txMip.data[0] = txRealloc(txMip.data[0], txMip.size);
      for (i=1; i<TX_MAX_LEVEL; i++) {
        txMip.data[i] = NULL;
      }
    } else {
      /* Allocate memory and generate mipmaps */
      int             depth;

      /*
       * How many levels to generate?
       *
       * -m  says "all" levels.
       * -m[1-12] says 1-12 levels, but see how many we can do actually.
       * -M <dim> says stop when smallest dim >= dim.
       */

      // printf("Mipmap levels : %d\n", txOptions.mipmap_lod);
      w = txMip.width;
      h = txMip.height;

      if (txOptions.mipmap_auto) {
                                // How many maximum levels can we generate?
        for (depth=1;  (w > 1) || (h > 1); depth++) {
          if (w > 1) w >>= 1;
          if (h > 1) h >>= 1;
        }
                                // printf("max depth = %d, miplod = %d\n", depth, txOptions.mipmap_lod);
        if (txOptions.mipmap_lod != -1) {
          // Depth explicitly specified by user.
          if (txOptions.mipmap_lod < depth)
            depth = txOptions.mipmap_lod;
        }
                                // printf("max depth = %d, miplod = %d\n", depth, txOptions.mipmap_lod);
      } else {
                                // Stop when smallest mip level's dimension > mipmap_lod.
        w = (w > h) ? w : h;

        for (depth=1;  w > txOptions.mipmap_lod; depth++) {
          w >>= 1;
        }
      }

      /* 
       * At this point, depth tells me how many mipmap levels
       * (including level 0) to generate. Allocate memory, and
       * setup the data[i] pointers.
       */
      // printf("Generating depth %d\n", depth);

      if (depth > txMip.depth) {
                                // Need to generate mipmaps.

                                // How much memory do we need?
        txMip.depth = depth;
        w = txMip.width;
        h = txMip.height;
        txMip.size = 0;
        for (i=0; i< depth; i++) {
          txMip.size += w * h * 4;
          if (w > 1) w >>= 1;
          if (h > 1) h >>= 1;
        }

                                // Allocate this memory, and set up data pointers.
        txMip.data[0] = txRealloc(txMip.data[0], txMip.size);
        w = txMip.width;
        h = txMip.height;
        for (i=1; i<txMip.depth; i++) {
          txMip.data[i] = (FxU8 *)txMip.data[i-1] + w * h * 4;
          if (w > 1) w >>= 1;
          if (h > 1) h >>= 1;
        }

        for (i=txMip.depth; i < TX_MAX_LEVEL; i++)
          txMip.data[i] = NULL;

        txMipMipmap(&txMip);
      } else {
                                // We should probably re-malloc this, but it's smaller.
        txMip.depth = depth;
        printf("Using original mipmaps. (%d levels)\n", txMip.depth);

        for (i=txMip.depth; i<= TX_MAX_LEVEL; i++)
          txMip.data[i] = NULL;
      }
    }

    /************** QUANTIZE, DITHER *************/
    if (txOptions.view_en) {

      printf("Viewing Resampled, mipmapped image.\n");
      txMipView(&txMip, ifile, 0, txOptions.view_background);
    }

    pxMip.format = txOptions.pixel_format;

    pxMip.width  = txMip.width;

    pxMip.height = txMip.height;
    pxMip.depth  = txMip.depth;
    pxMip.size   = txMip.size * GR_TEXFMT_SIZE(pxMip.format)/
      GR_TEXFMT_SIZE(txMip.format);

    pxMip.data[0] = txMalloc( pxMip.size);
    w = pxMip.width;
    h = pxMip.height;
    for (i=1; i<TX_MAX_LEVEL; i++) {
      if (i >= pxMip.depth) {
        pxMip.data[i] = NULL;
        continue;
      }
      pxMip.data[i] = (FxU8 *) pxMip.data[i-1] + w * h *
        GR_TEXFMT_SIZE(pxMip.format);

      if (w > 1) w >>= 1;
      if (h > 1) h >>= 1;
    }
    txMipQuantize(&pxMip, &txMip, txOptions.pixel_format, 
                  txOptions.dither, txOptions.compression);

    printf("Dequantize for viewing.\n");
    txMipDequantize(&txMip, &pxMip);

    /**************** VIEW **********************/
    if (txOptions.view_en) {
      printf("Viewing final image.\n");
      txMipView(&txMip, ifile, 1, txOptions.view_background);
    }

    /**************** WRITE *********************/
    if (txOptions.output_en) {
      char    filebase[80];
      char    *obase, *oext;

      // printf("Writing\n");
      if (txOptions.output_base[0] == 0) {
        txBasename(ifile, filebase);
        obase = filebase;
      } else {

        obase = txOptions.output_base;
      }
      oext = txOptions.output_ext;

      // printf("Output Base :%s: Ext :%s:\n", obase, oext);

      if (0) {
      } else if (strcmp(oext, ".3df") == 0) {
        txMipWrite(&pxMip, obase, oext, txOptions.output_split);
      } else if (strcmp(oext, ".tga") == 0) {
        txMipWrite(&txMip, obase, oext, txOptions.output_split);
      }
    } else {
      printf("No output file generated\n");
    }
  }
  txViewClose();
}
