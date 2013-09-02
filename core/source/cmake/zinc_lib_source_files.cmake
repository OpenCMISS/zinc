
# Defines ZINC_LIB_SRCS
SET( ZINC_LIB_BASE_SRCS  ${COMFILE_SRCS} ${CONTEXT_SRCS} ${GENERAL_SRCS}
	${COMPUTED_FIELD_SRCS} ${CURVE_SRCS} ${ELEMENT_SRCS} ${EMOTER_SRCS}
	${FIELD_IO_SRCS} ${FINITE_ELEMENT_GRAPHICS_SRCS} ${FINITE_ELEMENT_ADDITIONAL_SRCS}
	${GRAPHICS_SRCS} ${IMAGE_PROCESSING_SRCS} ${INTERACTION_SRCS}
	${IO_DEVICES_SRCS} ${MESH_SRCS} ${NODE_SRCS} ${SELECTION_SRCS} ${INTERPRETER_SRCS}
	${THREE_D_DRAWING_SRCS} ${TIME_SRCS} ${USER_INTERFACE_SRCS} ${OPENCASCADE_SRCS}
	${FINITE_ELEMENT_CORE_SRCS} ${REGION_SRCS} )
SET( ZINC_LIB_BASE_HDRS ${API_HDRS} ${API_TYPES_HDRS} ${APIPP_HDRS} ${COMFILE_HDRS} ${CONTEXT_HDRS}
	${COMPUTED_FIELD_HDRS} ${CURVE_HDRS} ${ELEMENT_HDRS} ${EMOTER_HDRS} ${GENERAL_HDRS}
	${FIELD_IO_HDRS} ${FINITE_ELEMENT_GRAPHICS_HDRS} ${FINITE_ELEMENT_ADDITIONAL_HDRS}
	${GRAPHICS_HDRS} ${IMAGE_PROCESSING_HDRS} ${INTERACTION_HDRS}
	${IO_DEVICES_HDRS} ${MESH_HDRS} ${NODE_HDRS} ${SELECTION_HDRS} ${INTERPRETER_HDRS}
	${THREE_D_DRAWING_HDRS} ${TIME_HDRS} ${USER_INTERFACE_HDRS} ${OPENCASCADE_HDRS}
	${FINITE_ELEMENT_CORE_HDRS} ${REGION_HDRS} )


SET( ZINC_LIB_SRCS
		${ZINC_LIB_BASE_SRCS} )

SET( ZINC_LIB_HDRS
		${ZINC_LIB_BASE_HDRS} )
