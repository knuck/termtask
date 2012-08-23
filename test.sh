tail -f /tmp/dzenesis |
while true
do
  tr "\n" ":" |
  cat
done
