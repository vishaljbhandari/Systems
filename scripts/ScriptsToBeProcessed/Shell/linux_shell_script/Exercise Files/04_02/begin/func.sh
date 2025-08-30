#!/usr/bin/env bash

function Hello() {
  echo "Hello"
}

Goodbye() {
  echo "Goodbye"
}

echo "Calling the Hello function"
Hello

echo "Calling the Goodbye function"
Goodbye

exit 0
