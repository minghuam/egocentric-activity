#include <vector>
#include <string>
#include <iostream>

#include "Poco/File.h"
#include "Poco/Path.h"

void ls_dir(std::vector<string> &files, std::string path, bool recursive = false){
	Poco::File dir(path);
	if(!dir.exists() || !dir.isDirectory()){
		std::cerr << "Not a valid directory: " << path << std::endl;
		return;
	}

	std::vector<Poco::File> poco_files;
	dir.list(poco_files);

	for(const Poco::File &poco_file : poco_files){
		if(poco_file.isHidden()){
			continue;
		}
		files.push_back(poco_file.path());
		if(poco_file.isDirectory() && recursive){
			ls_dir(files, poco_file.path(), recursive);
		}
	}
}

std::vector<std::string> ls_files(std::string path, std::string extension)

std::vector<std::string> ls_images(std::string path, bool recursive = false, std::string extension = "jpg"){
	std::vector<std::string> ret;

	Poco::File dir(path);
	if(!dir.exists() || !dir.isDirectory()){
		std::cerr << "Not a valid directory: " << path << std::endl;
		return ret;
	}

	std::vector<Poco::File> files;
	dir.list(files);

	for(const Poco::File &file : files){
		if(file.isHidden()){
			continue;
		}

		if(file.isDirectory()){
			if(recursive){
				std::vector<std::string> imgs = ls_images(file.path(), recursive, extension);
				for(const std::string &img : imgs){
					ret.push_back(img);
				}
			}
		}else{
			Poco::Path p(file.path());
			if(p.getExtension() == extension){
				ret.push_back(file.path());
			}
		}
	}

	return ret;
}


int main(int argc, char** argv){

	std::vector<std::string> imgs = ls_images("./data/images", true);
	for(std::string img : imgs){
		std::cout << img << std::endl;
	}

}