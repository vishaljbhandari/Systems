echo "Ruby CODE"
echo "========="
wc -l `find . -name "*.rb" | grep -v vendor  | grep -v test | grep -v migrate | grep -v config\/ | grep -v public\/  `

echo "RHTML CODE"
echo "=========="
wc -l `find . -name "*.rhtml" `

echo "JAVASCRIPT CODE"
echo "==============="
wc -l `find . -name "*.js" | grep -v jacob.draw2d  | grep -v test | grep -v help_files `
