#!/bin/bash

IMAGE_NAME="dev-env-gsisi"
CONTAINER_NAME="dev-env-gsisi-container"
CONTAINER_ENTRY="/usr/bin/bash"
WORK_DIR="/home/dev/gsisi"

PROJECT_ROOT="$(pwd)"

function container_stop() {
	sudo docker stop $CONTAINER_NAME || echo "container already stopped"
}

function clean() {
	container_stop
	sudo docker rm $CONTAINER_NAME || echo "container already removed"
	sudo docker image rm $IMAGE_NAME  || echo "image already removed"
}

function container_start() {
	if [ -z "$(sudo docker ps -a | grep $CONTAINER_NAME)" ]; then
		echo ">> creating dev container"

		sudo docker run -w $WORK_DIR --hostname dev -it --name $CONTAINER_NAME \
			--net=host \
			--cap-add=SYS_PTRACE --security-opt seccomp=unconfined \
			-v $PROJECT_ROOT:$WORK_DIR:z \
			$IMAGE_NAME $CONTAINER_ENTRY
	else
		echo ">> found existing dev container"
		sudo docker start $CONTAINER_NAME
		sudo docker exec -it $CONTAINER_NAME $CONTAINER_ENTRY
	fi
}

function image_build() {
	if [ -z "$(sudo docker images | grep $IMAGE_NAME)" ]; then
		echo ">> building image: $IMAGE_NAME"
		sudo docker build -t $IMAGE_NAME ./
	fi
}

function print_usage() {
	echo "Usage: ./docker.sh [container-start | container-stop | image-build | clean]"
}

function main() {
	if ! command -v docker >/dev/null 2>&1; then
		echo "Could not find docker. Please install it https://docs.docker.com/engine/install/"
		exit 1
	fi

	case "$1" in
	  container-start) container_start ;;
	  container-stop)  container_stop ;;
	  image-build)     image_build ;;
	  clean)           clean ;;
	  *)               print_usage ;;
	esac
}

main $@