#! /bin/sh -ue

repo=$1
user=${2:degustaf}

git clone https://github.com/degustaf/yarl "$repo"
cd "$repo"
git remote rename origin upstream
git remote add origin "https://github.com/${user}/${repo}.git"
git push origin nonTCOD
