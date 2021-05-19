# Always install to install tree - packaging requires this even if virtualenv is used.

install(TARGETS ${SWIG_MODULE_TARGETS} ${ZINC_SHARED_TARGET}
    DESTINATION ${PYTHON_MODULE_TARGETS_DESTINATION_PREFIX}
    COMPONENT PythonBindings
)
install(FILES $<TARGET_FILE:zinc> ${_GENERATOR_EXPRESSION_ZINC_SONAME}
    DESTINATION ${PYTHON_MODULE_TARGETS_DESTINATION_PREFIX}
    COMPONENT PythonBindings
)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}${CFG_INSTALL_DIR}/setup.py
    DESTINATION ${PYTHON_DESTINATION_PREFIX}
    COMPONENT PythonBindings
)
install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}${CFG_INSTALL_DIR}/opencmiss
    DESTINATION ${PYTHON_DESTINATION_PREFIX}
    COMPONENT PythonBindings
    FILES_MATCHING PATTERN "*.py"
)
install(FILES "${BASE_PYTHON_PACKAGE_DIR}/README.rst"
    DESTINATION ${PYTHON_DESTINATION_PREFIX}
    COMPONENT PythonBindings
)

# Install step for virtual environment (if given)
if (ZINC_USE_VIRTUALENV)
    # Variables required for OCPythonBindingsVirtualEnv.
    set(VIRTUALENV_INSTALL_PREFIX ${ZINC_VIRTUALENV_INSTALL_PREFIX})
    set(PYTHON_PACKAGE_CURRENT_NAME zinc)
    # Virtual environment creation target.
    include(OCPythonBindingsVirtualEnv)
endif ()

