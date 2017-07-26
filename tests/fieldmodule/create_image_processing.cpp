/*
 * OpenCMISS-Zinc Library Unit Tests
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <gtest/gtest.h>

#include <opencmiss/zinc/core.h>
#include <opencmiss/zinc/field.h>
#include <opencmiss/zinc/fieldcache.h>
#include <opencmiss/zinc/fieldcomposite.h>
#include <opencmiss/zinc/fieldconstant.h>
#include <opencmiss/zinc/fieldimage.h>
#include <opencmiss/zinc/fieldimageprocessing.h>
#include <opencmiss/zinc/fieldmodule.h>
#include <opencmiss/zinc/region.h>
#include <opencmiss/zinc/stream.h>
#include <opencmiss/zinc/streamimage.h>

#include <opencmiss/zinc/field.hpp>
#include <opencmiss/zinc/fieldimage.hpp>
#include <opencmiss/zinc/fieldimageprocessing.hpp>
#include <opencmiss/zinc/fieldmodule.hpp>
#include <opencmiss/zinc/streamimage.hpp>

#include "zinctestsetup.hpp"
#include "zinctestsetupcpp.hpp"
#include "test_resources.h"

#if defined (ZINC_USE_ITK)
TEST(cmzn_fieldmodule_create_field_imagefilter_connected_threshold, invalid_args)
{
	const double values[] = { 0.3, 0.1, 0.7 };
	ZincTestSetup zinc;

	cmzn_field_id f1 = cmzn_fieldmodule_create_field_imagefilter_curvature_anisotropic_diffusion(zinc.fm, 0, 0.2, 1.0, 1);
	EXPECT_EQ((cmzn_field_id)0, f1);

	cmzn_field_id f2 = cmzn_fieldmodule_create_field_imagefilter_connected_threshold(zinc.fm, 0, 0.2, 1.0, 1.0, 1, 3, values);
	EXPECT_EQ((cmzn_field_id)0, f2);

}

TEST(cmzn_fieldmodule_create_field_imagefilter_curvature_anisotropic_diffusion, valid_args)
{
	ZincTestSetup zinc;

	// Create empty image field
	cmzn_field_id f1 = cmzn_fieldmodule_create_field_image(zinc.fm);
	EXPECT_NE(static_cast<cmzn_field_id>(0), f1);

	cmzn_field_image_id im = cmzn_field_cast_image(f1);
	cmzn_streaminformation_id si = cmzn_field_image_create_streaminformation_image(im);
	EXPECT_NE(static_cast<cmzn_streaminformation_id>(0), si);

	cmzn_streamresource_id sr = cmzn_streaminformation_create_streamresource_file(si, TestResources::getLocation(TestResources::TESTIMAGE_GRAY_JPG_RESOURCE));

	cmzn_streaminformation_image_id si_image = cmzn_streaminformation_cast_image(si);
	EXPECT_NE(static_cast<cmzn_streaminformation_image_id>(0), si_image);

	EXPECT_EQ(CMZN_OK, cmzn_field_image_read(im, si_image));

	cmzn_field_id f2 = cmzn_fieldmodule_create_field_imagefilter_curvature_anisotropic_diffusion(zinc.fm, cmzn_field_image_base_cast(im), 0.1, 1.0, 1);
	EXPECT_NE((cmzn_field_id)0, f2);

//	cmzn_field_id f2 = cmzn_fieldmodule_create_field_connected_threshold_image_filter(zinc.fm, f1, 0.2, 1.0, 1.0, 1, 3, values);
//	EXPECT_NE((cmzn_field_id)0, f2);

	cmzn_field_id xi = cmzn_field_image_get_domain_field(im);
	EXPECT_NE((cmzn_field_id)0, xi);

	cmzn_fieldcache_id cache = cmzn_fieldmodule_create_fieldcache(zinc.fm);

	double location[] = { 0.7, 0.2};
	double value = 0.0;

	EXPECT_EQ(CMZN_OK, cmzn_fieldcache_set_field_real(cache, xi, 2, location));
//	EXPECT_EQ(CMZN_OK, cmzn_field_evaluate_real(cmzn_field_image_base_cast(im), cache, 1, &value));

	EXPECT_EQ(CMZN_OK, cmzn_field_evaluate_real(f2, cache, 1, &value));
	EXPECT_NEAR(0.211765, value, 1e-6);

	cmzn_fieldcache_destroy(&cache);
	cmzn_field_destroy(&xi);
	cmzn_field_destroy(&f1);
	cmzn_field_destroy(&f2);
	cmzn_streamresource_destroy(&sr);
	cmzn_streaminformation_image_destroy(&si_image);
	cmzn_streaminformation_destroy(&si);
	cmzn_field_image_destroy(&im);

}

TEST(cmzn_fieldmodule_create_field_imagefilter_connected_threshold, valid_args)
{
	ZincTestSetup zinc;
	double location[] = { 0.7, 0.2};

	// Create empty image field
	cmzn_field_id f1 = cmzn_fieldmodule_create_field_image(zinc.fm);
	EXPECT_NE(static_cast<cmzn_field_id>(0), f1);

	cmzn_field_image_id im = cmzn_field_cast_image(f1);
	cmzn_streaminformation_id si = cmzn_field_image_create_streaminformation_image(im);
	EXPECT_NE(static_cast<cmzn_streaminformation_id>(0), si);

	cmzn_streaminformation_image_id si_image = cmzn_streaminformation_cast_image(si);
	EXPECT_NE(static_cast<cmzn_streaminformation_image_id>(0), si_image);

	cmzn_streamresource_id sr = cmzn_streaminformation_create_streamresource_file(si, TestResources::getLocation(TestResources::TESTIMAGE_GRAY_JPG_RESOURCE));

	EXPECT_EQ(CMZN_OK, cmzn_field_image_read(im, si_image));

	cmzn_field_id f2 = cmzn_fieldmodule_create_field_imagefilter_connected_threshold(zinc.fm, cmzn_field_image_base_cast(im), 0.2, 0.22, 0.33, 1, 2, location);
	EXPECT_NE((cmzn_field_id)0, f2);

	cmzn_field_id xi = cmzn_field_image_get_domain_field(im);
	EXPECT_NE((cmzn_field_id)0, xi);

	cmzn_fieldcache_id cache = cmzn_fieldmodule_create_fieldcache(zinc.fm);

	double value = 0.0;
	EXPECT_EQ(CMZN_OK, cmzn_fieldcache_set_field_real(cache, xi, 2, location));
//	EXPECT_EQ(CMZN_OK, cmzn_field_evaluate_real(cmzn_field_image_base_cast(im), cache, 1, &value));

	EXPECT_EQ(CMZN_OK, cmzn_field_evaluate_real(f2, cache, 1, &value));
	EXPECT_NEAR(0.33, value, 1e-6);

	cmzn_fieldcache_destroy(&cache);
	cmzn_field_destroy(&xi);
	cmzn_field_destroy(&f1);
	cmzn_field_destroy(&f2);
	cmzn_streamresource_destroy(&sr);
	cmzn_streaminformation_image_destroy(&si_image);
	cmzn_streaminformation_destroy(&si);
	cmzn_field_image_destroy(&im);
}

TEST(cmzn_field_imagefilter_threshold, api)
{
	ZincTestSetup zinc;
	int result;

	cmzn_field_id f1 = cmzn_fieldmodule_create_field_image(zinc.fm);
	EXPECT_NE(static_cast<cmzn_field_id>(0), f1);
	cmzn_field_image_id im = cmzn_field_cast_image(f1);
	EXPECT_EQ(CMZN_OK, result = cmzn_field_image_read_file(im, TestResources::getLocation(TestResources::TESTIMAGE_GRAY_JPG_RESOURCE)));

	cmzn_field_id f2 = cmzn_fieldmodule_create_field_imagefilter_threshold(zinc.fm, f1);
	cmzn_field_imagefilter_threshold_id th = cmzn_field_cast_imagefilter_threshold(f2);

	cmzn_field_imagefilter_threshold_condition condition;
	EXPECT_EQ(CMZN_FIELD_IMAGEFILTER_THRESHOLD_CONDITION_BELOW, condition = cmzn_field_imagefilter_threshold_get_condition(th));
	EXPECT_EQ(CMZN_OK, result = cmzn_field_imagefilter_threshold_set_condition(th, CMZN_FIELD_IMAGEFILTER_THRESHOLD_CONDITION_OUTSIDE));
	EXPECT_EQ(CMZN_FIELD_IMAGEFILTER_THRESHOLD_CONDITION_OUTSIDE, condition = cmzn_field_imagefilter_threshold_get_condition(th));

	double value;
	ASSERT_DOUBLE_EQ(0.0, value = cmzn_field_imagefilter_threshold_get_outside_value(th));
	EXPECT_EQ(CMZN_OK, result = cmzn_field_imagefilter_threshold_set_outside_value(th, 1.0));
	ASSERT_DOUBLE_EQ(1.0, value = cmzn_field_imagefilter_threshold_get_outside_value(th));

	ASSERT_DOUBLE_EQ(0.5, value = cmzn_field_imagefilter_threshold_get_lower_threshold(th));
	EXPECT_EQ(CMZN_OK, result = cmzn_field_imagefilter_threshold_set_lower_threshold(th, 0.2));
	ASSERT_DOUBLE_EQ(0.2, value = cmzn_field_imagefilter_threshold_get_lower_threshold(th));

	ASSERT_DOUBLE_EQ(0.5, value = cmzn_field_imagefilter_threshold_get_upper_threshold(th));
	EXPECT_EQ(CMZN_OK, result = cmzn_field_imagefilter_threshold_set_upper_threshold(th, 0.8));
	ASSERT_DOUBLE_EQ(0.8, value = cmzn_field_imagefilter_threshold_get_upper_threshold(th));

	cmzn_field_imagefilter_threshold_destroy(&th);
	cmzn_field_destroy(&f1);
	cmzn_field_destroy(&f2);
	cmzn_field_image_destroy(&im);
}

TEST(ZincFieldImagefilterThreshold, api)
{
	ZincTestSetupCpp zinc;
	int result;

	FieldImage im = zinc.fm.createFieldImage();
	EXPECT_TRUE(im.isValid());
	EXPECT_EQ(CMZN_OK, result = im.readFile(TestResources::getLocation(TestResources::TESTIMAGE_GRAY_JPG_RESOURCE)));

	FieldImagefilterThreshold th = zinc.fm.createFieldImagefilterThreshold(im);
	EXPECT_TRUE(th.isValid());

	// test casting
	FieldImagefilterThreshold tmp = th.castImagefilterThreshold();
	EXPECT_TRUE(tmp.isValid());

	FieldImagefilterThreshold::Condition condition;
	EXPECT_EQ(FieldImagefilterThreshold::CONDITION_BELOW, condition = th.getCondition());
	EXPECT_EQ(CMZN_OK, result = th.setCondition(FieldImagefilterThreshold::CONDITION_OUTSIDE));
	EXPECT_EQ(FieldImagefilterThreshold::CONDITION_OUTSIDE, condition = th.getCondition());

	double value;
	ASSERT_DOUBLE_EQ(0.0, value = th.getOutsideValue());
	EXPECT_EQ(CMZN_OK, result = th.setOutsideValue(1.0));
	ASSERT_DOUBLE_EQ(1.0, value = th.getOutsideValue());

	ASSERT_DOUBLE_EQ(0.5, value = th.getLowerThreshold());
	EXPECT_EQ(CMZN_OK, result = th.setLowerThreshold(0.2));
	ASSERT_DOUBLE_EQ(0.2, value = th.getLowerThreshold());

	ASSERT_DOUBLE_EQ(0.5, value = th.getUpperThreshold());
	EXPECT_EQ(CMZN_OK, result = th.setUpperThreshold(0.8));
	ASSERT_DOUBLE_EQ(0.8, value = th.getUpperThreshold());
}
#endif
