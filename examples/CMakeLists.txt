# OptiWeave Examples

# Basic transformation example
add_executable(basic_transformation_example
    basic_transformation/example.cpp
)

target_link_libraries(basic_transformation_example PRIVATE optiweave_core)

# Install examples
install(TARGETS basic_transformation_example
    DESTINATION bin/examples
)