#pragma once
#include "File.h"
#include "iSearchable.h"
#include "iShareable.h"

class TextFile : public File, public iSearchable, public iShareable
{
    
};
