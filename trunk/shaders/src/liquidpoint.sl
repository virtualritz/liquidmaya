light liquidpoint(
    uniform float  intensity     = 1;
    uniform color  lightcolor    = 1;
    uniform float decay          = 0;

    uniform string shadownamepx  = "";
    uniform string shadownamenx  = "";
    uniform string shadownamepy  = "";
    uniform string shadownameny  = "";
    uniform string shadownamepz  = "";
    uniform string shadownamenz  = "";

    uniform float  shadowbias    = 0.01;
    uniform float  shadowblur    = 0.0;
    uniform float  shadowsamples = 16;
    uniform float  shadowfiltersize = 1;
    uniform color  shadowcolor   = 0;

    output varying color __shadow = 0;
    output varying color __unshadowed_Cl = 0;
    output float __nondiffuse    = 0;
    output float __nonspecular   = 0;
)
{
  illuminate( point "shader" ( 0, 0, 0 ) ) {

    if ( shadownamepx == "raytrace" ) {
      __shadow = shadow( shadownamepx, Ps, "samples", shadowsamples, "blur", shadowfiltersize*0.2+shadowblur, "bias", shadowbias, "width", 1 );
    } else {
      vector Lworld = vtransform( "world", L );

      float Lx = xcomp( Lworld );
      float LxAbs = abs( Lx );
      float Ly = ycomp( Lworld );
      float LyAbs = abs( Ly );
      float Lz = zcomp( Lworld );
      float LzAbs = abs( Lz );

      if( ( LxAbs > LyAbs ) && ( LxAbs > LzAbs ) ) {
        if( ( Lx > 0 ) && ( shadownamepx != "" ) ) {
          uniform float shadowsize[2];
          textureinfo( shadownamepx, "resolution", shadowsize );
          __shadow = shadow( shadownamepx, Ps, "samples", shadowsamples, "blur", shadowfiltersize*1/shadowsize[0]+shadowblur, "bias", shadowbias, "width", 1 );
        } else if( shadownamenx != "") {
          uniform float shadowsize[2];
          textureinfo( shadownamenx, "resolution", shadowsize );
          __shadow = shadow( shadownamenx, Ps, "samples", shadowsamples, "blur", shadowfiltersize*1/shadowsize[0]+shadowblur, "bias", shadowbias, "width", 1 );
        }
      } else if( (LyAbs > LxAbs) && ( LyAbs > LzAbs ) ) {
        if( ( Ly > 0 ) && ( shadownamepy != "" ) ) {
          uniform float shadowsize[2];
          textureinfo( shadownamepy, "resolution", shadowsize );
          __shadow = shadow( shadownamepy, Ps, "samples", shadowsamples, "blur", shadowfiltersize*1/shadowsize[0]+shadowblur, "bias", shadowbias, "width", 1 );
        } else if( shadownameny != "" ) {
          uniform float shadowsize[2];
          textureinfo( shadownameny, "resolution", shadowsize );
          __shadow = shadow( shadownameny, Ps, "samples", shadowsamples, "blur", shadowfiltersize*1/shadowsize[0]+shadowblur, "bias", shadowbias, "width", 1 );
        }
      } else if( ( LzAbs > LyAbs ) && ( LzAbs > LxAbs ) ) {
        if( ( Lz > 0 ) && ( shadownamepz != "" ) ) {
          uniform float shadowsize[2];
          textureinfo( shadownamepz, "resolution", shadowsize );
          __shadow = shadow( shadownamepz, Ps, "samples", shadowsamples, "blur", shadowfiltersize*1/shadowsize[0]+shadowblur, "bias", shadowbias, "width", 1 );
        } else if( shadownamenz != "") {
          uniform float shadowsize[2];
          textureinfo( shadownamenz, "resolution", shadowsize );
          __shadow = shadow( shadownamenz, Ps, "samples", shadowsamples, "blur", shadowfiltersize*1/shadowsize[0]+shadowblur, "bias", shadowbias, "width", 1 );
        }
      } else
        __shadow = 0;
    }

    Cl = intensity * pow( 1 / length( L ), decay );
    __unshadowed_Cl = Cl * lightcolor;
#if defined ( DELIGHT ) || defined ( PRMAN )
    Cl *= mix( lightcolor, shadowcolor, __shadow );
#else
    Cl *= mix( lightcolor, shadowcolor, ( comp( __shadow, 0 ) + comp( __shadow, 1 ) + comp( __shadow, 2 ) ) / 3 );
#endif
  }
}
