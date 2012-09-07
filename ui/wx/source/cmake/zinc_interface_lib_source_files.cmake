

IF( WX_USER_INTERFACE )
        SET( ZINC_INTERFACE_LIB_SRCS ${API_INTERFACE_SRCS} ${COLOUR_INTERFACE_SRCS}
                ${COMFILE_INTERFACE_SRCS}
                ${COMMAND_INTERFACE_SRCS} ${COMPUTED_FIELD_INTERFACE_SRCS}
                ${CONTEXT_SRCS} ${GRAPHICS_INTERFACE_SRCS}
                ${MATERIAL_INTERFACE_SRCS} ${NODE_INTERFACE_SRCS}
                ${REGION_INTERFACE_SRCS} ${THREE_D_DRAWING_INTERFACE_SRCS}
                ${TRANSFORMATION_INTERFACE_SRCS} ${USER_INTERFACE_SRCS} )
        SET( ZINC_INTERFACE_LIB_HDRS ${API_INTERFACE_HDRS} ${COLOUR_INTERFACE_HDRS}
                ${COMFILE_INTERFACE_HDRS}
                ${COMMAND_INTERFACE_HDRS} ${COMPUTED_FIELD_INTERFACE_HDRS}
                ${CONTEXT_HDRS} ${GRAPHICS_INTERFACE_HDRS}
                ${MATERIAL_INTERFACE_HDRS} ${NODE_INTERFACE_HDRS}
                ${REGION_INTERFACE_HDRS} ${THREE_D_DRAWING_INTERFACE_HDRS}
                ${TRANSFORMATION_INTERFACE_HDRS} ${wxWidgets_GENERATED_HDRS}  ${USER_INTERFACE_HDRS} )
ENDIF( WX_USER_INTERFACE )

IF( WIN32_USER_INTERFACE )
        SET( ZINC_INTERFACE_LIB_SRCS ${API_INTERFACE_SRCS}
                ${COMMAND_INTERFACE_SRCS}
                ${COMPUTED_FIELD_INTERFACE_SRCS}
                ${GRAPHICS_INTERFACE_SRCS} )
        SET( ZINC_INTERFACE_LIB_HDRS ${API_INTERFACE_HDRS}
                ${COMMAND_INTERFACE_HDRS} ${COMPUTED_FIELD_INTERFACE_HDRS}
                ${GRAPHICS_INTERFACE_HDRS} )
ENDIF( WIN32_USER_INTERFACE )

IF( GTK_USER_INTERFACE )
    SET( ZINC_INTERFACE_LIB_SRCS ${API_INTERFACE_SRCS}
        ${COMMAND_INTERFACE_SRCS}
        ${COMPUTED_FIELD_INTERFACE_SRCS}
        ${GRAPHICS_INTERFACE_SRCS} )
    SET( ZINC_INTERFACE_LIB_HDRS ${API_INTERFACE_HDRS}
        ${COMMAND_INTERFACE_HDRS} ${COMPUTED_FIELD_INTERFACE_HDRS}
        ${GRAPHICS_INTERFACE_HDRS} )
ENDIF( GTK_USER_INTERFACE )
