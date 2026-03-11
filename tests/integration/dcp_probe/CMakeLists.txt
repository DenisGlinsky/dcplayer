add_test(NAME dcp_probe_help COMMAND $<TARGET_FILE:dcp_probe> --help)
set_tests_properties(dcp_probe_help PROPERTIES LABELS "integration;smoke;dcp_probe")
