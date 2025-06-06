//
// Created by Ali Elmorsy on 5/24/2025.
//

#ifndef FILEMANAGMENT_H
#define FILEMANAGMENT_H
#include <string>


namespace amara::platform {
    /**
     *
     * @param path Path to the file to be read
     * @return a string contains the source of a file
     */
    std::string readStringFileFromStorage(const std::string &path);

    /**
     * Read a file from the storage (assets file/...etc.) based on the platform
     * @param path Path of the file
     * @return a tuple where the first item is the unsigned byte pointer of the data
     *
    * An <b>IMPORTANT NOTE</b>: The returned data must be freed manually as it's a raw pointer.
     */
    std::tuple<uint8_t *, size_t> readBinaryFileFromStorage(const std::string &path);
}

#endif //FILEMANAGMENT_H
