set(FAASM_USER stream)

function(stream_func exec_name dir_path)
    faasm_func(${exec_name} ${dir_path})
    set(ALL_STREAM_FUNCS ${ALL_STREAM_FUNCS} ${exec_name} PARENT_SCOPE)
endfunction(stream_func)

stream_func(wc_split wc_split.cpp)
stream_func(wc_count wc_count.cpp)
stream_func(sd_moving_avg sd_moving_avg.cpp)
stream_func(sd_spike_detect sd_spike_detect.cpp)
stream_func(mo_score mo_score.cpp)
stream_func(mo_anomaly mo_anomaly.cpp)
stream_func(mo_alert mo_alert.cpp)

# Custom target to group all the demo functions
add_custom_target(stream_all_funcs DEPENDS ${ALL_STREAM_FUNCS})
