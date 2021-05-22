#!/usr/bin/bash
# Provides: mohican
# Required-View-Cmd: $SERVER_NAME $cmd $key
### END INIT INFO


export MOHICANS_HOME=.
export SERVER_NAME=mohican
export PID_FILE=pid_file.txt
export DEFAULT_PATH_TO_CONFIG=.mohican.conf


get_pid() {
  # shellcheck disable=SC2002
  PID_MASTER_PROCESS=$(head -n 1 "$PID_FILE")
}

start() {
  if [ -f "$PID_FILE" ]; then
    echo "Server has already started!"
    exit 1
  else
    echo "Starting $SERVER_NAME server..."
    "$MOHICANS_HOME"/mohican.out
    echo "Server started!"
    exit 0
  fi
}

stop_soft() {
  if [ ! -f "$PID_FILE" ]; then
    echo "Server has not started yet!"
    exit 1
  else
	  echo "Stopping soft $SERVER_NAME server..."
	  get_pid
	  kill -1 "$PID_MASTER_PROCESS"
	  echo "Server stopped!"
	  rm "$PID_FILE"
	  exit 0
	fi
}

stop_hard() {
  if [ ! -f "$PID_FILE" ]; then
    echo "Server has not started yet!"
    exit 1
  else
    echo "Stopping hard $SERVER_NAME server..."
    get_pid
    kill -2 "$PID_MASTER_PROCESS"
    echo "Server stopped!"
    rm "$PID_FILE"
    exit 0
  fi
}

reload() {
  if [ ! -f "$PID_FILE" ]; then
    echo "Server has not started yet!"
    exit 1
  else
    echo "Reloading $SERVER_NAME server..."
    get_pid
    :> "$PID_FILE"
    kill -13 "$PID_MASTER_PROCESS"
  fi
}

status() {
# shellcheck disable=SC2046
  if [ -f "$PID_FILE" ]; then
    echo "$SERVER_NAME is running!"
  else
    echo "$SERVER_NAME is down!"
  fi
}

create_config() {
  cp "$DEFAULT_PATH_TO_CONFIG" settings/mohican.conf
}

  #read -n 1 -p "(нажмите любую клавишу для продолжения)"
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
	    if [ -f "$PID_FILE" ]; then
	      echo "Usage : <stop|reload|status>";
	    else
	      echo "Usage : <start|status|create>";
	    fi
	  ;;
  esac
exit 0
