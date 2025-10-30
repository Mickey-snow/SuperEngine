
function(configure_stacktrace target)
  target_compile_definitions(${target} PUBLIC ASSERTX_USE_BOOST_STACKTRACE)
  target_compile_options(${target} PUBLIC -g)

  if (MSVC)
    target_compile_definitions(${target} PUBLIC BOOST_STACKTRACE_USE_WINDBG)
    target_compile_options(${target} PUBLIC /Zi /FS)
    target_link_options(${target} PUBLIC /DEBUG /INCREMENTAL)
    target_link_libraries(${target} PUBLIC DbgHelp)
    return()
  endif()

  # Prefer libbacktrace on Unix
  find_package(Boost 1.75 QUIET COMPONENTS stacktrace_backtrace)
  if(Boost_stacktrace_backtrace_FOUND)
    target_compile_definitions(${target} PUBLIC BOOST_STACKTRACE_USE_BACKTRACE)
    target_compile_options(${target} PUBLIC -fno-omit-frame-pointer)
    target_link_libraries(${target} PUBLIC Boost::stacktrace_backtrace backtrace dl)
    return()
  endif()

  # Fallback to addr2line
  find_package(Boost 1.75 QUIET COMPONENTS stacktrace_addr2line)
  if(Boost_stacktrace_addr2line_FOUND)
    target_compile_definitions(${target} PUBLIC BOOST_STACKTRACE_USE_ADDR2LINE)
    target_compile_options(${target} PUBLIC -fno-omit-frame-pointer)
    if(APPLE)
      target_link_options(${target} PUBLIC "-Wl,-export_dynamic")
    else()
      target_link_options(${target} PUBLIC "-rdynamic")
    endif()
    target_link_libraries(${target} PUBLIC Boost::stacktrace_addr2line)
    return()
  endif()

  # Last resort: basic
  find_package(Boost 1.75 REQUIRED COMPONENTS stacktrace_basic)
  target_compile_definitions(${target} PUBLIC BOOST_STACKTRACE_USE_BASIC)
  if(APPLE)
    target_link_options(${target} PUBLIC "-Wl,-export_dynamic")
  else()
    target_link_options(${target} PUBLIC "-rdynamic")
  endif()
  target_link_libraries(${target} PUBLIC Boost::stacktrace_basic)
endfunction()

