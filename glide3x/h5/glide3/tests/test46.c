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
*/

#include <assert.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <glide.h>
#ifdef FX_GLIDE_NAPALM
#include <g3ext.h>
#endif
#include "tlib.h"

int hwconfig;
static const char *version;
static tlGlideExtension glideext;

static const char name[]    = "test46";
static const char purpose[] = "tbuffer example";
static const char usage[]   = "-n <frames> -r <res> -d <filename> -p <pixel format>";

const char *cmodeName[] = {
  "OFF     ",
  "ON      "
};

void 
main(int argc, char **argv) 
{
  char match; 
  char **remArgs;
  int  rv;

  GrScreenResolution_t resolution = GR_RESOLUTION_640x480;
  float                scrWidth   = 640.0f;
  float                scrHeight  = 480.0f;
  int frames                      = -1;
  FxBool               scrgrab = FXFALSE;
  char                 filename[256];
  FxU8                 subframe = 0;

  TlTexture            texture;
  FxU32                zrange[2];
  GrContext_t          gc = 0;
  FxU32                pixelformat = 0x0003; /* 16 bit 565 */
  static FxBool        useCCExt = 1;
  FxU32                tmask = 1;

  float 
    xLeft,
    xRight,
    yTop,
    yBottom;

  /* Process Command Line Arguments */
  while(rv = tlGetOpt(argc, argv, "nrdxp", &match, &remArgs)) {
    if (rv == -1) {
      printf("Unrecognized command line argument\n");
      printf("%s %s\n", name, usage);
      printf("Available resolutions:\n%s\n",
             tlGetResolutionList());
      return;
    }
    switch(match) {
    case 'n':
      frames = atoi(remArgs[0]);
      break;
    case 'r':
      resolution = tlGetResolutionConstant(remArgs[0], 
                                           &scrWidth, 
                                           &scrHeight);
      break;
    case 'd':
      scrgrab = FXTRUE;
      frames = 5;      
      strcpy(filename, remArgs[0]);
      break;

    case 'p':
      pixelformat = tlGetPixelFormat( remArgs[0] );
      break;
    }
  }

  tlSetScreen(scrWidth, scrHeight);

  version = grGetString(GR_VERSION);

  printf("%s:\n%s\n", name, purpose);
  printf("%s\n", version);
  printf("Resolution: %s\n", tlGetResolutionString(resolution));
  if (frames == -1) {
    printf("Press A Key To Begin Test.\n");
    tlGetCH();
  }
    
  /* Initialize Glide */
  grGlideInit();
  assert(hwconfig = tlVoodooType());

  grGet(GR_ZDEPTH_MIN_MAX, 8, (FxI32 *)zrange);

  grSstSelect(0);

  tlInitGlideExt(&glideext);
  
  if (!glideext.combineext) {
    printf("Could not use COMBINE extension.\n");
    useCCExt = 0;
  }

  if (glideext.grSstWinOpen) {
    gc = glideext.grSstWinOpen(tlGethWnd(),
                               resolution,
                               GR_REFRESH_60Hz,
                               GR_COLORFORMAT_ABGR,
                               GR_ORIGIN_UPPER_LEFT,
                               pixelformat,
                               2, 1);
  } else {
    gc = grSstWinOpen(tlGethWnd(),
                      resolution,
                      GR_REFRESH_60Hz,
                      GR_COLORFORMAT_ABGR,
                      GR_ORIGIN_UPPER_LEFT,
                      2, 1);
  }
  if (!gc) {
    printf("Could not allocate glide fullscreen context.\n");
    goto __errExit;
  }
  
  tlVertexLayout();
  tlConSet(0.0f, 0.0f, 1.0f, 0.5f, 
           60, 15, 0xffffff);
    
  /* Load texture data into system ram */
  assert(tlLoadTexture("miro.3df", 
                       &texture.info, 
                       &texture.tableType, 
                       &texture.tableData));
  /* Download texture data to TMU */
  grTexDownloadMipMap(GR_TMU0,
                      grTexMinAddress(GR_TMU0),
                      GR_MIPMAPLEVELMASK_BOTH,
                      &texture.info);
  if (texture.tableType != NO_TABLE) {
    grTexDownloadTable(texture.tableType,
                       &texture.tableData);
  }

  /* Select Texture As Source of all texturing operations */
  grTexSource(GR_TMU0,
              grTexMinAddress(GR_TMU0),
              GR_MIPMAPLEVELMASK_BOTH,
              &texture.info);


  /* Enable Bilinear Filtering + Mipmapping */
  grTexFilterMode(GR_TMU0,
                  GR_TEXTUREFILTER_BILINEAR,
                  GR_TEXTUREFILTER_BILINEAR);
  grTexMipMapMode(GR_TMU0,
                  GR_MIPMAP_NEAREST,
                  FXFALSE);

  xLeft   = tlScaleX( 0.2f );
  xRight  = tlScaleX( 0.8f );
  yTop    = tlScaleY( 0.2f );
  yBottom = tlScaleY( 0.8f );

  while(frames-- && tlOkToRender()) {
    GrVertex vtxA, vtxB, vtxC, vtxD;
    FxU32 cont = 1;

    if (hwconfig == TL_VOODOORUSH) {
      tlGetDimsByConst(resolution,
                       &scrWidth, 
                       &scrHeight);
        
      grClipWindow(0, 0, (FxU32) scrWidth, (FxU32) scrHeight);
    }
    
    glideext.grTBufferWriteMask( tmask );    

    grBufferClear(0xff0000, 0, zrange[1]);

    tlConOutput(  "e   : toggle combine extension\n");
    tlConOutput("COMBINE extension %s\n", cmodeName[useCCExt]);
    tlConOutput(" tmask : %x\n", tmask);

    if (useCCExt) {
#ifdef FX_GLIDE_NAPALM
      glideext.grTexColorCombineExt(GR_TMU0,
                                    GR_CMBX_ZERO, GR_FUNC_MODE_X, 
                                    GR_CMBX_LOCAL_TEXTURE_RGB, GR_FUNC_MODE_X, 
                                    GR_CMBX_ZERO, FXFALSE,
                                    GR_CMBX_B, FXFALSE,
                                    0, FXFALSE);
      glideext.grColorCombineExt(GR_CMBX_TEXTURE_RGB, GR_FUNC_MODE_X, 
                                 GR_CMBX_ZERO, GR_FUNC_MODE_X, 
                                 GR_CMBX_ZERO, FXTRUE, 
                                 GR_CMBX_ZERO, FXFALSE,
                                 0, FXFALSE);
#endif
    }
    else {
      /* Set up Render State - Decal Texture - color combine set in render loop */
      grTexCombine(GR_TMU0,
                   GR_COMBINE_FUNCTION_LOCAL,
                   GR_COMBINE_FACTOR_NONE,
                   GR_COMBINE_FUNCTION_NONE,
                   GR_COMBINE_FACTOR_NONE,
                   FXFALSE, FXFALSE);
      
      grColorCombine(GR_COMBINE_FUNCTION_SCALE_OTHER,
                     GR_COMBINE_FACTOR_ONE,
                     GR_COMBINE_LOCAL_NONE,
                     GR_COMBINE_OTHER_TEXTURE,
                     FXFALSE);
    }
        
    /*---- 
      A-B
      |\|
      C-D
      -----*/
    vtxA.oow = 1.0f;
    vtxB = vtxC = vtxD = vtxA;

    vtxA.x = vtxC.x = xLeft;
    vtxB.x = vtxD.x = xRight;
    vtxA.y = vtxB.y = yTop;
    vtxC.y = vtxD.y = yBottom;

    vtxA.tmuvtx[0].sow = vtxC.tmuvtx[0].sow = 0.0f;
    vtxB.tmuvtx[0].sow = vtxD.tmuvtx[0].sow = 255.0f;
    vtxA.tmuvtx[0].tow = vtxB.tmuvtx[0].tow = 0.0f;
    vtxC.tmuvtx[0].tow = vtxD.tmuvtx[0].tow = 255.0f;

    vtxA.r = 255.0f, vtxA.g =   0.0f, vtxA.b =   0.0f, vtxA.a = 255.0f;
    vtxB.r =   0.0f, vtxB.g = 255.0f, vtxB.b =   0.0f, vtxB.a = 128.0f;
    vtxC.r =   0.0f, vtxC.g =   0.0f, vtxC.b = 255.0f, vtxC.a = 128.0f;
    vtxD.r =   0.0f, vtxD.g =   0.0f, vtxD.b =   0.0f, vtxD.a =   0.0f;

    grConstantColorValue(0xFF0000FFUL);
    grTexSource(GR_TMU0,
                grTexMinAddress(GR_TMU0),
                GR_MIPMAPLEVELMASK_BOTH,
                &texture.info);

    grDrawTriangle(&vtxA, &vtxD, &vtxC);
    grDrawTriangle(&vtxA, &vtxB, &vtxD);

    tlConRender();
    tlConClear();

    /* grab the frame buffer */
    if (scrgrab) {
      char fname[256], tmp[32];
      FxU16 cnt;

      cnt = strcspn(filename, ".");
      strncpy(fname, filename, cnt);
      fname[cnt] = 0;
      sprintf(tmp,"_%d\0", subframe);
      strcat(fname, tmp);
      strcat(fname, filename+cnt);
      if (!tlScreenDump(fname, (FxU16)scrWidth, (FxU16)scrHeight))
        printf("Cannot open %s\n", filename);

      subframe++;
    }
        
    do { /* while(tlKbHit()) { */
      const char curKey = tlGetCH();

      switch(curKey) {
      case 'c':
      case 'C':
        cont = 0;
        break;
      case 't':
      case 'T':
        cont = 0;
        tmask = tmask + 1;
        if (tmask > 0xf)
          tmask = 0;
        break;
      case 'e':
      case 'E':
        cont = 0;
        useCCExt = (!useCCExt);
        if (!glideext.combineext)
          useCCExt = 0;
        break;

      default:
        cont = 0;
        frames = 0;
        break;
      }
    } while ( cont );

    grBufferSwap(1);
    grFinish();

  }
    
 __errExit:    
  grGlideShutdown();
}
