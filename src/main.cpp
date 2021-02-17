// glSpect - OpenGL real-time spectrogram
// Alex Barnett 12/21/10, based on Luke Campagnola's nice 2005 glScope.
// ahb@math.dartmouth.edu
// Tweaked 10/13/11 for inverse video, etc.
// 1/25/14 fix -EPIPE snd_pcm_readi() error. nchannels=2, winf=0 case, etc
//        added color_byte to clean up color mapping; color map.
// 1/17/16: freq indicator line via left-button

/* Notes: be sure to set vSync wait in graphics card (eg NVIDIA) OpenGL settings
 */

/* ISSUES: (2011)
 * occasional dropped audio every few secs - why? (1470 vs 1472? issue)
 * jitter in signal graph scrolling
 * GlutGameMode isnt' setting refresh to 60Hz, rather 75Hz.
 * Better than glDrawPixels (which sends all data every frame to GPU) woudl be
    to pass the data as a texture and scroll it in the GPU (a convolution?),
    modifying only one row each time. This would be super low CPU usage!
 * Use GL_ARB_pixel_buffer_object for fast glDrawPixels or textures via DMA?
 * Add color?
 * add playback of audio file, or jack into audio playback?
 * glDrawpixels is deprecated in OpenGl >3.0. THat's annoying.
   Eg: https://www.opengl.org/discussion_boards/showthread.php/181907-drawing-individual-pixels-with-opengl
  "The modern way to do it is to store the data in a texture then draw a pair of textured triangles (quads are also deprecated)."
 */

/* SOLVED ISSUES: (2011)
 * Creation and initialization of global scn happens before main(), bad, since
     couldn't set t_memory in cmd line! Ans: use trivial creator/destructor,
     and call other init/close routines from main().
 */

#include "param.h"
#include "scene.h"

#include <linux/videodev2.h>

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <iostream>

#include <math.h>

#include <sys/ioctl.h>
#include <sys/time.h>

static int verb;    // global verbosity
Param param;        // global parameters object
static Scene scn;   // global scene which contains everything (eg via scn.ai)

static int v4l2sink = -1;
static char *vidsendbuf = NULL;
static int vidsendsiz = 0;

//  GLUT's display routine: 2D layering by write order
void display() {
    glClear(GL_COLOR_BUFFER_BIT); // no depth buffer
    
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, 1, 0, 1, -1, 1);  // l r b t n f
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    scn.spectrogram();  // note, overlays the signal and spectrum graphs
    // glFinish();   // wait for all gl commands to complete

    glutSwapBuffers(); // for this to WAIT for vSync, need enable in NVIDIA OpenGL
}

// --------------------------- IDLE FUNC ------------------------------------
void idle(void) {  //    put numerical intensive part here? only scrolling
    ++scn.frameCount;        // compute FPS...
    time_t now = time(NULL);     // integer in seconds
    if (now > scn.fps_tic) {    // advanced by at least 1 sec?
        scn.fps = scn.frameCount;
        scn.frameCount = 0;
        scn.fps_tic = now;
    }

    if (!scn.ai->pause) {  // scroll every scroll_fac vSyncs, if not paused:
        timeval nowe;  // update runtime
        gettimeofday(&nowe, NULL);

        scn.run_time += 1/60.0;                     // is less jittery, approx true
        scn.run_time = fmod(scn.run_time, 100.0);   // wrap time label after 100 secs

        ++scn.scroll_count;
        if (scn.scroll_count == scn.scroll_fac) {
            scn.scroll();   // add spec slice to sg & scroll
            scn.scroll_count = 0;
            if (vidsendsiz != write(v4l2sink, scn.ai->pixels, vidsendsiz))
                exit(-1);
        }
    }

    glutPostRedisplay();  // trigger GLUT display func
}


void keyboard(unsigned char key, int xPos, int yPos) {
    if( key == 27 || key == 'q') {  // esc or q to quit
        scn.ai->quitNow();
        exit(0);
    } 
    else if( key == ' ' ) {
        scn.pause = ! scn.pause;
        scn.ai->pause = ! scn.ai->pause;
    }
    else if( key == ']' ) {               // speed up scroll rate
        if (scn.scroll_fac>1) {
            scn.scroll_fac--; 
            scn.scroll_count=0;
        }
    }
    else if( key == '[' ) {
        if (scn.scroll_fac<50) 
            scn.scroll_fac++;
    }
    else {
        fprintf(stderr, "pressed key %d\n", (int)key);
    }
}

// special key handling
void special(int key, int xPos, int yPos) {
    if ( key == 102 )// rt
        scn.color_scale[1] *= 1.5;
    else if ( key == 100 ) // lt
        scn.color_scale[1] /= 1.5;
    else if ( key == 103 ) // dn
        scn.color_scale[0] -= 20;
    else if ( key == 101 ) // up
        scn.color_scale[0] += 20;

    std::cout << scn.color_scale[0] << "," << scn.color_scale[1] << std::endl;
}

void reshape(int w, int h) {// obsolete in game mode
    glViewport(0, 0, w, h);
}


#define vidioc(op, arg) \
	if (ioctl(dev_fd, VIDIOC_##op, arg) == -1) \
		sysfail(#op); \
	else

static void open_vpipe(char * _dev, int width, int height) {
    v4l2sink = open(_dev, O_WRONLY);
    if (v4l2sink < 0) {
        fprintf(stderr, "Failed to open v4l2sink device. (%s)\n", strerror(errno));
        exit(-2);
    }
    // setup video for proper format
    struct v4l2_format v;
    int t;
    v.type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
    t = ioctl(v4l2sink, VIDIOC_G_FMT, &v);
    if( t < 0 )
        exit(t);
    v.fmt.pix.width = width;
    v.fmt.pix.height = height;
    v.fmt.pix.pixelformat = V4L2_PIX_FMT_RGB24;
    v.fmt.pix.field = V4L2_FIELD_NONE;
	// v.fmt.pix.bytesperline = linewidth;
	// v.fmt.pix.colorspace = V4L2_COLORSPACE_SRGB;
    v.fmt.pix.sizeimage = width * height * 3;

    t = ioctl(v4l2sink, VIDIOC_S_FMT, &v);
    if (t < 0 )
        exit(t);

    vidsendsiz = width * height * 3;
    vidsendbuf = malloc( width * height * 3 );
}

// ===========================================================================
int main(int argc, char** argv) {
    verb = 0;          // 0 silent, 1 debug, etc
    param.windowtype = 2;  // Gaussian

    for (int i=1; i<argc; ++i) {  // .....Parse cmd line options....
        if (!strcmp(argv[i], "-v"))  // option -v makes verbose
            verb = 1;
        else if (!strcmp(argv[i], "-sf")) {
            sscanf(argv[++i], "%d", &scn.scroll_fac);  // read in scroll factor
            if (scn.scroll_fac < 1) 
                scn.scroll_fac = 1; // sanitize it
        } 
    }
    scn.init();       // true constructor for global scn object

    open_vpipe("/dev/video4", scn.ai->n_f, scn.ai->n_tw);

    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA);

    glutInitWindowSize(scn.ai->n_f, scn.ai->n_tw);  // window same size as XGA
    glutCreateWindow("sigmundAnalizer");

    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutKeyboardFunc(keyboard);
    glutSpecialFunc(special);
    glutIdleFunc(idle);

    glutMainLoop();
    return 0;
}
