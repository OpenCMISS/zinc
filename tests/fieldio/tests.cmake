# OpenCMISS-Zinc Library Unit Tests
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

SET(CURRENT_TEST fieldio)
LIST(APPEND API_TESTS ${CURRENT_TEST})
SET(${CURRENT_TEST}_SRC
	${CURRENT_TEST}/ex_io.cpp
	${CURRENT_TEST}/fieldml_basic.cpp
	${CURRENT_TEST}/fieldml_hermite.cpp
	utilities/fileio.cpp
	)

SET(FIELDIO_FIELDML_CUBE_RESOURCE "${CMAKE_CURRENT_LIST_DIR}/cube.fieldml")
SET(FIELDIO_FIELDML_TETMESH_RESOURCE "${CMAKE_CURRENT_LIST_DIR}/tetmesh.fieldml")
SET(FIELDIO_FIELDML_WHEEL_DIRECT_RESOURCE "${CMAKE_CURRENT_LIST_DIR}/wheel_direct.fieldml")
SET(FIELDIO_FIELDML_WHEEL_INDIRECT_RESOURCE "${CMAKE_CURRENT_LIST_DIR}/wheel_indirect.fieldml")
SET(FIELDIO_EX_LINES_UNIT_SCALE_FACTORS_RESOURCE "${CMAKE_CURRENT_LIST_DIR}/lines_unit_scale_factors.exfile")
SET(FIELDIO_EX_LINES_ALTERNATE_NODE_ORDER_RESOURCE "${CMAKE_CURRENT_LIST_DIR}/lines_alternate_node_order.exfile")
SET(FIELDIO_EX_LINES_INCONSISTENT_NODE_ORDER_RESOURCE "${CMAKE_CURRENT_LIST_DIR}/lines_inconsistent_node_order.exfile")
SET(FIELDIO_EX_HERMITE_FIGURE8_RESOURCE "${CMAKE_CURRENT_LIST_DIR}/figure8.exfile")
SET(FIELDIO_EX_TWOHERMITECUBES_NOSCALEFACTORS_RESOURCE "${CMAKE_CURRENT_LIST_DIR}/twohermitecubes_noscalefactors.exfile")
SET(FIELDIO_EX2_CUBE_RESOURCE "${CMAKE_CURRENT_LIST_DIR}/cube.ex2")
SET(FIELDIO_EX2_TETMESH_RESOURCE "${CMAKE_CURRENT_LIST_DIR}/tetmesh.ex2")
SET(FIELDIO_EX2_WHEEL_RESOURCE "${CMAKE_CURRENT_LIST_DIR}/wheel.ex2")
SET(FIELDIO_EX_PROLATE_HEART_RESOURCE "${CMAKE_CURRENT_LIST_DIR}/data/prolate_heart.exfile")
SET(FIELDIO_EX_HEMISPHERE_RESOURCE "${CMAKE_CURRENT_LIST_DIR}/data/hemisphere.exfile")
SET(FIELDIO_EX_CUBE_ELEMENT_XI_OLD_RESOURCE "${CMAKE_CURRENT_LIST_DIR}/data/cube_element_xi_old.exdata")
SET(FIELDIO_EX_CUBE_ELEMENT_XI_OLD_FAIL_RESOURCE "${CMAKE_CURRENT_LIST_DIR}/data/cube_element_xi_old_fail.exdata")
SET(FIELDIO_EX_BLOCK_GRID_RESOURCE "${CMAKE_CURRENT_LIST_DIR}/data/block_grid.exfile")
SET(FIELDIO_EX_SPECIAL_NODE_FIELDS_RESOURCE "${CMAKE_CURRENT_LIST_DIR}/data/special_node_fields.exnode")
SET(FIELDIO_EX2_CUBE_ELEMENT_RESOURCE "${CMAKE_CURRENT_LIST_DIR}/data/cube_element.ex2")
SET(FIELDIO_EX2_CUBE_NODE1_RESOURCE "${CMAKE_CURRENT_LIST_DIR}/data/cube_node1.ex2")
SET(FIELDIO_EX2_CUBE_NODE2_RESOURCE "${CMAKE_CURRENT_LIST_DIR}/data/cube_node2.ex2")
SET(FIELDIO_EX2_CUBE_NODE3_RESOURCE "${CMAKE_CURRENT_LIST_DIR}/data/cube_node3.ex2")
SET(FIELDIO_EX2_ALLSHAPES_ELEMENT_CONSTANT_RESOURCE "${CMAKE_CURRENT_LIST_DIR}/data/allshapes_element_constant.ex2")
SET(FIELDIO_EXF_DATA_AND_NODE_GROUP_RESOURCE "${CMAKE_CURRENT_LIST_DIR}/data/data_and_node_group.exf")
