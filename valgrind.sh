#!/bin/bash
valgrind -v --tool=memcheck --track-origins=yes --leak-check=full ./main
