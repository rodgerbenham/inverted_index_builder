#!/bin/bash
valgrind --track-origins=yes --leak-check=full ./main
