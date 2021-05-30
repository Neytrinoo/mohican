#!/usr/bin/bash
# Provides: mohican
# Required-View-Cmd: $SERVER_NAME $cmd $key
### END INIT INFO


export MOHICANS_HOME=.
export SERVER_NAME=mohican
export PID_FILE=pid_file.txt
export DEFAULT_PATH_TO_CONFIG=.mohican.conf


get_pid() {
  PID_MASTER_PROCESS=$(head -n 1 "$PID_FILE")
}

get_has_server_started() {
  HAS_SERVER_STARTED=$(ps aux | grep ./mohican.out | wc -l)
}

start() {
  get_has_server_started
  if [ "$HAS_SERVER_STARTED" \> 1 ]; then
    echo "Server has already started!"
    exit 1
  else
    rm error.log
    rm access.log
    touch access.log
    touch error.log
    echo "Starting $SERVER_NAME Server..."
    rm mohican.out
    cp ./cmake-build-debug/mohican.out mohican.out
    "$MOHICANS_HOME"/mohican.out
    echo "Server started!"
    exit 0
  fi
}

stop_soft() {
  get_has_server_started
  if [ ! "$HAS_SERVER_STARTED" \> 1 ]; then
    echo "Server has not started yet!"
    exit 1
  else
    echo "Stopping soft $SERVER_NAME server..."
    get_pid
    kill -1 "$PID_MASTER_PROCESS"
    echo "Server stopped!"
    exit 0
	fi
}

stop_hard() {
  get_has_server_started
  if [ ! "$HAS_SERVER_STARTED" \> 1 ]; then
    echo "Server has not started yet!"
    exit 1
  else
    echo "Stopping hard $SERVER_NAME server..."
    get_pid
    kill -2 "$PID_MASTER_PROCESS"
    echo "Server stopped!"
    exit 0
  fi
}

reload_soft() {
  get_has_server_started
  if [ ! "$HAS_SERVER_STARTED" \> 1 ]; then
    echo "Server has not started yet!"
    exit 1
  else
    echo "Reloading soft $SERVER_NAME server..."
    get_pid
    kill -13 "$PID_MASTER_PROCESS"
  fi
}

reload_hard() {
  get_has_server_started
  if [ ! "$HAS_SERVER_STARTED" \> 1 ]; then
    echo "Server has not started yet!"
    exit 1
  else
    echo "Reloading hard $SERVER_NAME server..."
    get_pid
    kill -14 "$PID_MASTER_PROCESS"
  fi
}

status() {
  get_has_server_started
  if [ "$HAS_SERVER_STARTED" \> 1 ]; then
    echo "$SERVER_NAME is running!"
  else
    echo "$SERVER_NAME is down!"
  fi
}

create_config() {
  cp "$DEFAULT_PATH_TO_CONFIG" settings/mohican.conf
}

case $1 in
  start)
    start
  ;;
  stop)
  shift
  case $1 in
    soft)
      stop_soft
      exit 0
    ;;
    hard)
      stop_hard
      exit 0
    ;;
  esac
  echo "Usage : <hard|soft>";
  ;;
  reload)
  shift
  case $1 in
    soft)
      reload_soft
      exit 0
    ;;
    hard)
      reload_hard
      exit 0
    ;;
  esac
  echo "Usage : <hard|soft>";
  ;;
  status)
    status
  ;;
  create)
    shift
    if [ "$1" =  config ]; then
      create_config
    else
      echo "Usage : <config>";
    fi
  ;;

  *)
    if [ "$(ps aux | grep ./mohican.out | wc -l)" \> 1 ]; then
      echo "Usage : <stop|reload|status>";
    else
      echo "Usage : <start|status|create>";
    fi
  ;;
esac
exit 0
