light liquiddistant(
   float intensity = 1;
   color lightcolor = 1;

   string shadowname = "";  /* shadow map name or "raytrace" for traced shadows */
   float shadowbias = 0.01;
   float shadowblur = 0.0;
   float shadowsamples = 16; /* samples or rays */
   uniform float shadowfiltersize = 1;
   color shadowcolor = 0;

   output varying color __shadow = 0;
   output varying color __unshadowed_Cl = 0;
   output float __nondiffuse = 0;  /* set to 1 to exclude from diffuse light */
   output float __nonspecular = 0; /* set to 1 to exclude from highlights */
)
{
  uniform float factor;
  if( shadowname != "" ) {
    if ( shadowname == "raytrace" ) factor = 0.2;
    else {
      uniform float shadowsize[2];
      textureinfo( shadowname, "resolution", shadowsize );
      factor = 1/shadowsize[0];
    }
  }

  solar( vector "shader" ( 0, 0, 1 ), 0 ) {

    if( shadowname != "" ) {
      uniform float shadowsize[2];
      textureinfo( shadowname, "resolution", shadowsize );
      __shadow = shadow( shadowname, Ps, "samples", shadowsamples, "blur", shadowfiltersize*factor, "bias", shadowbias, "width", 1 );
    } else
      __shadow = 0;

    Cl = intensity;
    __unshadowed_Cl = Cl * lightcolor;
#if defined ( DELIGHT ) || defined ( PRMAN )
    Cl *= mix( lightcolor, shadowcolor, __shadow );
#else
    Cl *= mix( lightcolor, shadowcolor, ( comp( __shadow, 0 ) + comp( __shadow, 1 ) + comp( __shadow, 2 ) ) / 3 );
#endif
  }

}
