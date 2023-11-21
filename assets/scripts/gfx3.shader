freezeShader
{
	nopicmip
	deformvertexes wave 100 sin 3 0 0 0

	{
		map textures/effects/envmap.tga
		blendfunc gl_one gl_one
		rgbgen const ( 0.20 0.20 0.20 )
		tcgen environment
	}

	{
		map gfx/ice/icechunks.tga
		blendfunc gl_one gl_one
		rgbgen const ( 0.15 0.15 0.15 )
		tcmod scale 4 4
	}
}

freezeShader2
{
	nopicmip

	{
		map textures/effects/envmap.tga
		blendfunc gl_one gl_one
		rgbgen const ( 0.20 0.20 0.20 )
		tcgen environment
	}

	{
		map gfx/ice/icechunks.tga
		blendfunc gl_one gl_one
		rgbgen const ( 0.15 0.15 0.15 )
		tcmod scale 4 4
	}
}

freezeShader2_nocull
{
	nopicmip
	cull none

	{
		map textures/effects/envmap.tga
		blendfunc gl_one gl_one
		rgbgen const ( 0.20 0.20 0.20 )
		tcgen environment
	}

	{
		map gfx/ice/icechunks.tga
		blendfunc gl_one gl_one
		rgbgen const ( 0.15 0.15 0.15 )
		tcmod scale 4 4
	}
}

bbox
{
	nopicmip

	{
		map gfx/misc/bbox.tga
		blendFunc GL_ONE GL_ONE
		rgbGen vertex
	}
}

bbox_nocull
{
	nopicmip
	cull none

	{
		map gfx/misc/bbox.tga
		blendFunc GL_ONE GL_ONE
		rgbGen vertex
	}
}

snowflake
{
	nopicmip
	sort nearest

	{
		clampmap gfx/misc/raildisc_mono2.tga 
		blendFunc GL_ONE GL_ONE
		rgbGen vertex
	}
}

rain
{
	nopicmip
	cull none
	sort nearest

	{
		map gfx/misc/raindrop.tga
		blendFunc GL_ONE GL_ONE
		rgbGen vertex
	}
}

freezeMarkShader
{
	nopicmip
	polygonoffset
	{
		clampmap gfx/damage/freeze_stain.tga
		blendfunc gl_src_alpha gl_one_minus_src_alpha
		rgbgen identitylighting
		alphagen vertex
	}
}

gfx/3d/crosshair
{
	nopicmip
	{
		map gfx/2d/crosshair.tga          
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA                
        	rgbGen entity
	}
}

gfx/3d/crosshairb
{
	nopicmip
	{
		map gfx/2d/crosshairb.tga
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbGen entity
	}
}

gfx/3d/crosshairc
{
	nopicmip
	{
		map gfx/2d/crosshairc.tga
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbGen entity
	}
}

gfx/3d/crosshaird
{
	nopicmip
	{
		map gfx/2d/crosshaird.tga
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbGen entity
	}
}

gfx/3d/crosshaire
{
	nopicmip
	{
		map gfx/2d/crosshaire.tga
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbGen entity
	}
}

gfx/3d/crosshairf
{
	nopicmip
	{
		map gfx/2d/crosshairf.tga
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbGen entity
	}
}

gfx/3d/crosshairg
{
	nopicmip
	{
		map gfx/2d/crosshairg.tga
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbGen entity
	}
}

gfx/3d/crosshairh
{
	nopicmip
	{
		map gfx/2d/crosshairh.tga
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbGen entity
	}
}

gfx/3d/crosshairi
{
	nopicmip
	{
		map gfx/2d/crosshairi.tga
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbGen entity
	}

}
gfx/3d/crosshairj
{
	nopicmip
	{
		map gfx/2d/crosshairj.tga       
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbGen entity
	}
}
gfx/3d/crosshairk
{
	nopicmip
	{
		map gfx/2d/crosshairk.tga       
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbGen entity
	}
}
