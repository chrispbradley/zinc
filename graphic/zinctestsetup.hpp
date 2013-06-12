#ifndef __ZINCTESTSETUP_HPP__
#define __ZINCTESTSETUP_HPP__

#include <gtest/gtest.h>

#include <zinc/zincconfigure.h>
#include <zinc/status.h>
#include <zinc/context.h>
#include <zinc/region.h>
#include <zinc/fieldmodule.h>
#include <zinc/glyph.h>
#include <zinc/graphicsmodule.h>
#include <zinc/rendition.h>

class ZincTestSetup
{
public:
	Cmiss_context_id context;
	Cmiss_region_id root_region;
	Cmiss_field_module_id fm;
	Cmiss_graphics_module_id gm;
	Cmiss_glyph_module_id glyph_module;
	Cmiss_rendition_id ren;

	ZincTestSetup() :
		context(Cmiss_context_create("test")),
		root_region(Cmiss_context_get_default_region(context)),
		fm(Cmiss_region_get_field_module(root_region)),
		gm(Cmiss_context_get_default_graphics_module(context)),
		glyph_module(Cmiss_graphics_module_get_glyph_module(gm)),
		ren(0)
	{
		EXPECT_EQ(CMISS_OK, Cmiss_graphics_module_enable_renditions(gm, root_region));
		ren = Cmiss_graphics_module_get_rendition(gm, root_region);
		EXPECT_NE(static_cast<Cmiss_field_module *>(0), fm);
		EXPECT_NE(static_cast<Cmiss_graphics_module *>(0), gm);
		EXPECT_NE(static_cast<Cmiss_glyph_module *>(0), glyph_module);
		EXPECT_EQ(CMISS_OK, Cmiss_glyph_module_create_standard_glyphs(glyph_module));
		EXPECT_NE(static_cast<Cmiss_rendition *>(0), ren);
	}

	~ZincTestSetup()
	{
		Cmiss_rendition_destroy(&ren);
		Cmiss_glyph_module_destroy(&glyph_module);
		Cmiss_graphics_module_destroy(&gm);
		Cmiss_field_module_destroy(&fm);
		Cmiss_region_destroy(&root_region);
		Cmiss_context_destroy(&context);
	}
};

#endif // __ZINCTESTSETUP_HPP__
