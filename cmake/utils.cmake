# Generate ragel file
function(ragelmaker src_rl outputlist outputdir)
  #Create a custom build step that will call ragel on the provided src_rl file.
  #The output .cpp file will be appended to the variable name passed in outputlist.

  get_filename_component(src_file ${src_rl} NAME_WE)
  set(rl_out ${outputdir}/${src_file}.cpp)
  set(${outputlist} ${${outputlist}} ${rl_out} PARENT_SCOPE)

  add_custom_command(
    OUTPUT ${rl_out}
    COMMAND cd ${outputdir}
    COMMAND ragel ${CMAKE_CURRENT_SOURCE_DIR}/${src_rl} -o ${rl_out} -L -l -C -F1 -G2
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/${src_rl}
    )
  set_source_files_properties(${rl_out} PROPERTIES GENERATED TRUE)
endfunction(ragelmaker)
