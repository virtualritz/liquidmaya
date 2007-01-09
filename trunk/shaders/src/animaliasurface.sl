

color adjustHSV( color input; float hue, saturation, lightness ) {

}

surface animaliasurface(
	uniform string TextureMap
	uniform string LightMap
	uniform string ShadowMap





) {

	color Ccolor = texture( TextureMap );
	color Cshadow = texture( ShadowMap );
	color Clight = texture( LightMap );

	textureName = TextureMap;
	concat( textureName, "-light.tif.tdl" );
	shadowName = ShadowMap;
	concat( shadowName "-shadow.tif.tdl" );

}
