#include <stdint.h>
#include <stdio.h>
#include <math.h>
#include <string.h>
#include <unistd.h>

#define PI 3.14159265

const float theta_spacing = 0.07;
const float phi_spacing   = 0.02;

// Radius of circle
const float R1 = 1;
// Center of circle
const float R2 = 2;
// Distance from object to screen
const float K2 = 5;
// Distance from eye to screen
const float K1 = 20;
const uint16_t screen_width = 80;
const uint16_t screen_height = 30;

static const char luminance[] = ".,-~:;=!*#$@";

void render_frame(float A, float B) {
  char output[screen_width * screen_height];
  float zbuffer[screen_width * screen_height];
  const int lum_count = (int)(sizeof(luminance) - 1);
  float sinA = sinf(A);
  float sinB = sinf(B);
  float cosA = cosf(A);
  float cosB = cosf(B);

  memset(output, ' ', sizeof(output));
  memset(zbuffer, 0, sizeof(zbuffer));
  // theta goes around the cross-sectional circle of a torus
  for (float theta=0; theta < 2*PI; theta += theta_spacing) {
    for(float phi=0; phi < 2*PI; phi += phi_spacing) {
      // precompute
      float sinphi = sinf(phi);
      float sintheta = sinf(theta);
      float costheta = cosf(theta);
      float cosphi = cosf(phi);
    
      // the x,y coordinate of the circle, before revolving (factored
      // out of the above equations)
      float circlex = R2 + R1*costheta;
      float circley = R1*sintheta;

      // final 3D (x,y,z) coordinate after rotations, directly from
      // our math above
      float x = circlex*(cosB*cosphi + sinA*sinB*sinphi)
        - circley*cosA*sinB; 
      float y = circlex*(sinB*cosphi - sinA*cosB*sinphi)
        + circley*cosA*cosB;
      float z = K2 + cosA*circlex*sinphi + circley*sinA;
      float ooz = 1/z;  // "one over z"
      
      // x and y projection.  note that y is negated here, because y
      // goes up in 3D space but down on 2D displays.
      int xp = (int) (screen_width/2 + 2*K1*ooz*x);
      int yp = (int) (screen_height/2 - K1*ooz*y);
      int i = xp + screen_width * yp;
      // calculate luminance.  ugly, but correct.
      float L = cosphi*costheta*sinB - cosA*costheta*sinphi -
        sinA*sintheta + cosB*(cosA*sintheta - costheta*sinA*sinphi);
      // L ranges from -sqrt(2) to +sqrt(2).  If it's < 0, the surface
      int lum_index = (int)(L * 8);
      if (lum_index < 0) {
        lum_index = 0;
      } else if (lum_index >= lum_count) {
        lum_index = lum_count - 1;
      }
      if (screen_height > yp && yp > 0 && xp > 0 && screen_width > xp && ooz > zbuffer[i]) {
        zbuffer[i] = ooz;
        // luminance_index is now in the range 0..11 (8*sqrt(2) = 11.3)
        // now we lookup the character corresponding to the
        // luminance and plot it in our output:
        output[i] = luminance[lum_index];
      }
    }
  }

  // now, dump output[] to the screen.
  // bring cursor to "home" location, in just about any currently-used
  // terminal emulation mode
  printf("\x1b[H");
  for (int k = 0; k < screen_width * screen_height + 1; k++) {
    putchar(k % screen_width ? output[k] : 10);
  }
}

int main(void) {
  float A = 0;
  float B = 0;

  printf("\x1b[2J");
  for (;;) {
    render_frame(A, B);
    A += 0.04f;
    B += 0.02f;
    usleep(15000);
  }
  return 0;
}
