git status --porcelain

git add -A
git commit -m "WIP: save work before flatten"

git branch backup/flat-$(date +%Y%m%d-%H%M%S)

git checkout --orphan flat-temp
git add -A
git commit -m "Flatten: single commit preserving current tree"

git branch -D main
git branch -m main
git checkout main

git push --force-with-lease origin main
