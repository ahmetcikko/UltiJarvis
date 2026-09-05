if(WIN32)
    execute_process(
        COMMAND taskkill /IM Ulti-Jarvis-Daemon.exe /F
        OUTPUT_QUIET
        ERROR_QUIET
    )
else()
    execute_process(
        COMMAND pkill -f Ulti-Jarvis-Daemon
        OUTPUT_QUIET
        ERROR_QUIET
    )
endif()
