@echo OFF
echo "Unzip."
tar -xf deploy_file.zip
echo "Unzip done."

echo "Copy"
rsync -arv ./deploy_file/ ./bin
echo "Copy done."