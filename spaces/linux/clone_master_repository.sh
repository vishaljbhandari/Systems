#!/bin/bash
CURRENT_DIR='pwd'
mkdir $REPO_HOME
cd $REPO_HOME

echo -n "Cloning Linux Space Repository"
git clone git@github.com:vishaljbhandari/LinuxSpace.git

echo -n "Cloning LEARN CPP Repository"
git clone git@github.com:vishaljbhandari/learn_cpp.git

echo -n "Cloning CPP AI Repository"
git clone git@github.com:vishaljbhandari/CppAI.git

echo -n "Cloning Environment Box Repository"
git clone git@github.com:vishaljbhandari/EnvironmentBox.git

echo -n "Cloning LEARN AI-ML Repository"
git clone git@github.com:vishaljbhandari/LearnAI-ML.git

echo -n "Cloning CLASSES CPP Repository"
git clone git@github.com:vishaljbhandari/ClassesCpp.git

echo -n "Cloning UNREAL ENGINE CPP Repository"
git clone git@github.com:vishaljbhandari/UnrealEngineCpp.git

echo -n "Cloning LEARN LINUX Repository"
git clone git@github.com:vishaljbhandari/learn_linux.git

echo -n "Cloning LEARN Java Repository"
git clone git@github.com:vishaljbhandari/learn_java.git

echo -n "Cloning LEARN C Repository"
git clone git@github.com:vishaljbhandari/learn_c.git

echo -n "Cloning LEARN Scripts Repository"
git clone git@github.com:vishaljbhandari/learn_scripts.git

echo -n "Cloning System Scripts C Repository"
git clone git@github.com:vishaljbhandari/system_scripts.git

echo -n "Cloning Projects Repository"
git@github.com:vishaljbhandari/Projects.git

echo -n "Cloning Simple Examples Repository"
git clone git@github.com:vishaljbhandari/simple_examples.git

echo -n "Cloning Workspace Repository"
git clone git@github.com:vishaljbhandari/workspace.git

echo -n "Cloning Classes CPP Repository"
git clone git@github.com:vishaljbhandari/classes_cpp.git

echo -n "Cloning System Scripts Repository"
git clone git@github.com:vishaljbhandari/SystemScripts.git


echo -n "Cloning Sample Examples Repository"
git@github.com:vishaljbhandari/SampleExamples.git

cd $CURRENT_DIR
