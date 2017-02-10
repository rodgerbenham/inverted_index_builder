#!/bin/bash
valgrind --tool=memcheck --track-origins=yes --leak-check=full ./main
