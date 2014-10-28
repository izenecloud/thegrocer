include_directories(
    ${CMAKE_INCLUDE_DIRECTORY}
    ${izenecma_INCLUDE_DIR}
)
add_executable(gen_train gen_train.cc)
target_link_libraries(gen_train ${Boost_LIBRARIES} ${izenecma_LIBRARIES})
add_executable(thegrocer thegrocer.cc)
target_link_libraries(thegrocer ${Boost_LIBRARIES} ${izenecma_LIBRARIES})
