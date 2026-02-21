#version 330 core
out uint FragID;

uniform uint objectID;

void main() {
	FragID = objectID;
}