plasmaExplosion_nomip
{
	nopicmip
	cull disable
	{
		clampmap models/weaphits/plasmaboom.tga
		blendfunc add
		tcMod stretch triangle .6 0.1 0 8
		tcmod rotate 999
		rgbGen wave inversesawtooth 0 1 0 1.5
	}
}

sprites/plasma1_nomip
{
	nopicmip
	cull disable
	{
		clampmap sprites/plasmaa.tga
		blendfunc GL_ONE GL_ONE
		tcMod rotate 931
	}
}

railExplosion_nomip
{
	nopicmip
	cull disable
	{
		animmap 5 models/weaphits/ring02_1.tga  models/weaphits/ring02_2.tga  models/weaphits/ring02_3.tga models/weaphits/ring02_4.tga gfx/colors/black.tga
		alphaGen wave inversesawtooth 0 1 0 5
		blendfunc blend
	}
	{
		animmap 5 models/weaphits/ring02_2.tga  models/weaphits/ring02_3.tga models/weaphits/ring02_4.tga gfx/colors/black.tga gfx/colors/black.tga
		alphaGen wave sawtooth 0 1 0 5
		blendfunc blend
	}
}

models/weaphits/bnomip1
{
	nopicmip
	cull disable
	{
		map models/weaphits/bullet1.tga
		rgbgen identity
	}
}

models/weaphits/bnomip2
{
	nopicmip
	cull disable
	{
		map models/weaphits/bullet1.tga
		rgbgen identity
	}
}

models/weaphits/bnomip3
{
	nopicmip
	cull disable
	{
		map models/weaphits/bullet1.tga
		rgbgen identity
	}
}

bulletExplosion_nomip
{
	nopicmip
	cull disable
	{
		animmap 5 models/weaphits/bullet1.tga  models/weaphits/bullet2.tga  models/weaphits/bullet3.tga gfx/colors/black.tga
		rgbGen wave inversesawtooth 0 1 0 5
		blendfunc add
	}
	{
		animmap 5 models/weaphits/bullet2.tga  models/weaphits/bullet3.tga  gfx/colors/black.tga gfx/colors/black.tga
		rgbGen wave sawtooth 0 1 0 5
		blendfunc add
	}
}

rocketExplosion_nomip
{
	nopicmip
	cull disable
	{
		animmap 8 models/weaphits/rlboom/rlboom_1.tga  models/weaphits/rlboom/rlboom_2.tga models/weaphits/rlboom/rlboom_3.tga models/weaphits/rlboom/rlboom_4.tga models/weaphits/rlboom/rlboom_5.tga models/weaphits/rlboom/rlboom_6.tga models/weaphits/rlboom/rlboom_7.tga models/weaphits/rlboom/rlboom_8.tga
		rgbGen wave inversesawtooth 0 1 0 8
		blendfunc add
	}
	{
		animmap 8 models/weaphits/rlboom/rlboom_2.tga models/weaphits/rlboom/rlboom_3.tga models/weaphits/rlboom/rlboom_4.tga models/weaphits/rlboom/rlboom_5.tga models/weaphits/rlboom/rlboom_6.tga models/weaphits/rlboom/rlboom_7.tga models/weaphits/rlboom/rlboom_8.tga gfx/colors/black.tga
		rgbGen wave sawtooth 0 1 0 8
		blendfunc add
	}
}

grenadeExplosion_nomip
{
	nopicmip
	cull disable
	{
		animmap 5 models/weaphits/glboom/glboom_1.tga  models/weaphits/glboom/glboom_2.tga models/weaphits/glboom/glboom_3.tga
		rgbGen wave inversesawtooth 0 1 0 5
		blendfunc add
	}
	{
		animmap 5 models/weaphits/glboom/glboom_2.tga  models/weaphits/glboom/glboom_3.tga gfx/colors/black.tga
		rgbGen wave sawtooth 0 1 0 5
		blendfunc add
	}
}

bfgExplosion_nomip
{
	nopicmip
	cull disable
	{
		animmap 5 models/weaphits/bfgboom/bfgboom_1.tga  models/weaphits/bfgboom/bfgboom_2.tga models/weaphits/bfgboom/bfgboom_3.tga
		rgbGen wave inversesawtooth 0 1 0 5
		blendfunc add
	}
	{
		animmap 5 models/weaphits/bfgboom/bfgboom_2.tga models/weaphits/bfgboom/bfgboom_3.tga gfx/colors/black.tga
		rgbGen wave sawtooth 0 1 0 5
		blendfunc add
	}
}

models/weaphits/bfgn1 // would be bfg01_nomip but I hex edited the model
{
	nopicmip
	deformVertexes autoSprite
	cull none

	{
		clampmap models/weaphits/bfg01.tga
		blendFunc GL_ONE GL_ONE
		tcMod rotate 333
		rgbGen identity
	}
	{
		clampmap models/weaphits/bfg01.tga
		blendFunc GL_ONE GL_ONE
		tcMod rotate -100
		rgbGen identity
	}
}

models/weaphits/bfgn2 // would be bfg02_nomip but I hex edited the model
{
	nopicmip
	cull none
	nomipmaps

	{
		map models/weaphits/bfg03.tga
		blendFunc GL_ONE GL_ONE
		tcmod scroll 2 0
		rgbGen identity
	}
	{
		map models/weaphits/bfg02.tga
		blendFunc GL_ONE GL_ONE
		tcmod scroll 3 0
		tcMod turb 0 .25 0 1.6
		rgbGen identity
	}
}

bloodExplosion_nomip		// spurt of blood at point of impact
{
	nopicmip
	cull disable
	{
		animmap 5 models/weaphits/blood201.tga models/weaphits/blood202.tga models/weaphits/blood203.tga models/weaphits/blood204.tga models/weaphits/blood205.tga
		blendfunc blend
	}
}

lightningBoltNew_nomip
{
	nopicmip
	cull none
	{
		map gfx/misc/lightning3new.tga
		blendFunc GL_ONE GL_ONE
		rgbgen wave sin 1 0.5 0 7.1
		tcmod scale  2 1
		tcMod scroll -5 0
	}
	{
		map gfx/misc/lightning3new.tga
		blendFunc GL_ONE GL_ONE
		rgbgen wave sin 1 0.8 0 8.1
		tcmod scale  -1.3 -1
		tcMod scroll -7.2 0
	}
}

models/weaphits/elenomip // would be electric_nomip but I hex edited the model file it goes to
{
	nopicmip
	cull none
	{
		clampmap models/weaphits/electric.tga
		blendFunc GL_ONE GL_ONE
		rgbgen wave triangle .8 2 0 9
		tcMod rotate 360
	}	
	{
		clampmap models/weaphits/electric.tga
		blendFunc GL_ONE GL_ONE
		rgbgen wave triangle 1 1.4 0 9.5
		tcMod rotate -202
	}	
}

railDisc_nomip
{
	nopicmip
	sort nearest
	cull none
        deformVertexes wave 100 sin 0 .5 0 2.4
	{
		clampmap gfx/misc/raildisc_mono2.tga 
		blendFunc GL_ONE GL_ONE
		rgbGen vertex
                 tcMod rotate -30
	}
}

railCore_nomip
{
	nopicmip
	sort nearest
	cull none
	{
		map gfx/misc/railcorethin_mono.tga
		blendFunc GL_ONE GL_ONE
		rgbGen vertex
		tcMod scroll -1 0
	}
}

smokePuff_nomip
{
	nopicmip
	cull none
	entityMergable		// allow all the sprites to be merged together
	{
		map gfx/misc/smokepuff3.tga
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbGen		vertex
		alphaGen	vertex
	}
}

hasteSmokePuff_nomip			// drops behind player's feet when speeded
{
	nopicmip
	cull none
	entityMergable		// allow all the sprites to be merged together
	{
		map gfx/misc/smokepuff3.tga
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
                //blendfunc GL_ONE GL_ONE
		rgbGen		vertex
		alphaGen	vertex
	}
}

smokePuffRagePro_nomip
{
	nopicmip
	cull none
	entityMergable		// allow all the sprites to be merged together
	{
		map gfx/misc/smokepuffragepro.tga
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
	}
}

shotgunSmokePuff_nomip
{
	nopicmip
	cull none
	{
		map gfx/misc/smokepuff2b.tga
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		alphaGen entity		
		rgbGen entity
	}
}
