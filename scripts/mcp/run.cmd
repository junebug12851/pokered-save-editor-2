@echo off
rem Launcher for the pokered-dev MCP server (stdio). Point the Claude MCP config at
rem this file. It must print NOTHING of its own to stdout — stdout is the protocol.
rem If the venv is missing, it bootstraps it first (output to stderr).
setlocal
set REPO=%~dp0..\..
set PY=%REPO%\tmp\mcp-venv\Scripts\python.exe
if not exist "%PY%" (
  powershell -NoProfile -ExecutionPolicy Bypass -File "%~dp0setup.ps1" 1>&2
)
"%PY%" "%~dp0pse_dev_mcp.py"
