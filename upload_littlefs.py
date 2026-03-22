Import("env")
import os

def after_upload(source, target, env):
    print("Uploading LittleFS...")
    env.Execute("pio run --target uploadfs")

env.AddPostAction("upload", after_upload)