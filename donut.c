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

void render_frame(float A, float B) {
  char output[screen_width * screen_height];
  float zbuffer[screen_width * screen_height];
  memset(output, ' ', screen_width * screen_height);
  memset(zbuffer, 0, screen_width * screen_height * 4);
  // theta goes around the cross-sectional circle of a torus
  for (float theta=0; theta < 2*PI; theta += theta_spacing) {
    for(float phi=0; phi < 2*PI; phi += phi_spacing) {
      // precompute
      float sinA = sin(A);
      float sinB = sin(B);
      float cosA = cos(A);
      float cosB = cos(B);
      float sinphi = sin(phi);
      float sintheta = sin(theta);
      float costheta = cos(theta);
      float cosphi = cos(phi);
    
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
      int lum_index = L * 8;
      if (screen_height > yp && yp > 0 && xp > 0 && screen_width > xp && ooz > zbuffer[i]) {
        zbuffer[i] = ooz;
          // luminance_index is now in the range 0..11 (8*sqrt(2) = 11.3)
          // now we lookup the character corresponding to the
          // luminance and plot it in our output:
          output[i] = ".,-~:;=!*#$@"[lum_index > 0 ? lum_index : 0];
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
int main() {
    float A =
        0, B = 0, i, j, zbuffer[screen_width * screen_height];
    printf("/x1b[2J]");
    int back = 0;
    for (;;) {
        render_frame(A,B);
        A += 0.04;
        B += 0.02;
        usleep(15000);
    }
    return 0;
}