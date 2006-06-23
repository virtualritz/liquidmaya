
#ifdef PRMAN
#include "/opt/pixar/RenderManProServer-13.0/lib/shaders/normals.h"
#else

normal shadingnormal(normal Ne){
	normal Ns;
	uniform float sides = 2;
	attribute("Sides",sides);
	if(sides == 2)	Ns = faceforward(normalize(Ne),I,Ne);
	else			Ns = normalize(Ne);
	return Ns;
}

#endif

/*
 * Rectangle light, an example of a "pseudo area light".
 * The rectangle is in the x-y plane (centered around the origin) and
 * has area xsize * ysize.
 * The sample positions are randomly distributed on a rectangle;
 * they are chosen in a stratified manner.
 * Occlusion is computed with ray tracing.
 * The illuminate point is chosen such that L.N is 1 in the diffuse
 * calculation in the surface shader.
 */
light
liquidarea(
            uniform float   intensity     = 1;
            uniform color   lightcolor    = 1;
            uniform float   decay         = 2;
            uniform string  coordsys      = "";
            uniform float   lightsamples  = 16;
            uniform float   doublesided   = 0;
            uniform string  shadowname    = "";
            uniform color   shadowcolor   = 0;

            output uniform float __nonspecular          = 1;
            output varying color __shadow               = 0;
            output varying color __unshadowed_Cl        = 0;
            output uniform float __arealightIntensity   = 0;
            output uniform color __arealightColor       = 0;
)
{
  // force non-specular
  __nonspecular = 1;

  // get the size of the coordinate system
  uniform float xsize = 1;
  uniform float ysize = 1;
  uniform point P1 = transform( coordsys, point "shader" (1, 1, 1) );
  xsize = 2/xcomp( P1 );
  ysize = 2/ycomp( P1 );

  uniform float xsamples, ysamples, i, j;
  color c, test;
  normal Ns = shadingnormal(N);
  vector len, lnorm, Nl;
  float x, y, dist, dot;
  varying float orient = 1;
  point p;

  /* stratified sampling approach
     we will actually use more samples than the requested number */
  uniform float aspectRatio = xsize / ysize;
  uniform float sqr = sqrt(lightsamples);
  xsamples = min(lightsamples, max( 2, ceil(sqr*aspectRatio) ) );
  ysamples = min(lightsamples, max( 2, ceil(sqr/aspectRatio) ) );
  if ( xsamples == 2 ) ysamples /= 2;
  if ( ysamples == 2 ) xsamples /= 2;

  /* Compute illumination */
  illuminate ( Ps + Ns ) {  /* force execution independent of light location */

      for (j = 0; j < ysamples; j += 1)  {
        for (i = 0; i < xsamples; i += 1)  {

          /* Compute jittered point (x,y) on unit square */
          x = (i + random()) / xsamples;
          y = (j + random()) / ysamples;

          /* Compute point p on rectangle */
          p = point "shader" ((x - 0.5) * xsize, (y - 0.5) * ysize, 0);

          /* Compute distance from light point p to surface point Ps */
          len = p - Ps;
          dist = length(len);
          lnorm = len / dist;

          /* luminaire sidedness */
          if ( doublesided == 0 ) {
            Nl = ( p - point "shader" ((x - 0.5) * xsize, (y - 0.5) * ysize, 1) ) ;
            orient = clamp( (Nl.len)/dist, 0, 1);
          }

          /* Compute light from point p to surface point Ps */
          dot = lnorm.Ns;
          if ( dot > 0 && orient > 0 ) {
            c = intensity * lightcolor * orient;
            /* distance falloff */
            c *= pow(dist , -decay);
            /* Lambert's cosine law at the surface */
            c *= dot;
            __unshadowed_Cl = c;
            /* raytraced occlusion - only if the point is reasonnably lit */
            if ( shadowname == "raytrace" && (comp(c,0)+comp(c,1)+comp(c,2))>0.005  ) {
              __shadow = transmission(Ps, p);
			  #ifdef PRMAN
              c *= mix( shadowcolor, color(1), __shadow);
			  #else
			  c *= color(mix(comp(shadowcolor,0),1,comp(__shadow,0)),
			  			 mix(comp(shadowcolor,1),1,comp(__shadow,1)),
						 mix(comp(shadowcolor,2),1,comp(__shadow,2)));
			  #endif
            }
            Cl += c;
          }
        }
      }
      Cl /= xsamples * ysamples;
  }

  __arealightIntensity = intensity;
  __arealightColor     = lightcolor;

}
