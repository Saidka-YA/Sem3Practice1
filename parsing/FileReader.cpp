#include "./FileReader.h"

bool FileReader::openCsv(const fs::path& csvPath) {
    file.open(csvPath);

    string header;
    if (!getline(file, header)) {
        readError = true;
        finished = true;
        return false;
    }

    return true;
}

bool FileReader::nextRow(string& line) {
    while (!finished) {
        if (getline(file, line)) {
            return true;
        }

        if (file.bad() || !file.eof()) {
            readError = true;
            finished = true;
            return false;
        }

        file.close();
        ++currentFile;

        fs::path nextPath = path / (to_string(currentFile) + ".csv");
        if (!fs::exists(nextPath)) {
            finished = true;
            return false;
        }

        if (!openCsv(nextPath)) {
            return false;
        }
    }

    return false;
}

void FileReader::readLine(string& line, Vector<string>& fields) {
    fields.Clear();
    string currField;
    bool inQuotes = false;

    for (size_t i = 0; i < line.size(); i++) {
        if (line[i] == ',' && !inQuotes) {
            fields.pushBack(currField);
            currField.clear();
        } else if (inQuotes && line[i] == '"' && i+1 < line.size() && line[i + 1] == '"') {
            currField += line[i];
            i++;
            continue;
        } else if (line[i] == '"') {
            inQuotes = !inQuotes;
        } else {
            currField += line[i];
        }
    }

    fields.pushBack(currField);
}
