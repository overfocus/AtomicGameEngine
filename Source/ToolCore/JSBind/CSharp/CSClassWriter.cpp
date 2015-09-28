//
// Copyright (c) 2014-2015, THUNDERBEAST GAMES LLC All rights reserved
// LICENSE: Atomic Game Engine Editor and Tools EULA
// Please see LICENSE_ATOMIC_EDITOR_AND_TOOLS.md in repository root for
// license information: https://github.com/AtomicGameEngine/AtomicGameEngine
//

#include <Atomic/IO/FileSystem.h>

#include "../JSBind.h"
#include "../JSBModule.h"
#include "../JSBPackage.h"
#include "../JSBEnum.h"
#include "../JSBClass.h"
#include "../JSBFunction.h"

#include "CSTypeHelper.h"
#include "CSClassWriter.h"
#include "CSFunctionWriter.h"

namespace ToolCore
{

CSClassWriter::CSClassWriter(JSBClass *klass) : JSBClassWriter(klass)
{

}

void CSClassWriter::WriteNativeFunctions(String& source)
{
    for (unsigned i = 0; i < klass_->functions_.Size(); i++)
    {
        JSBFunction* function = klass_->functions_.At(i);

        if (function->Skip())
            continue;

        if (function->IsDestructor())
            continue;

        if (OmitFunction(function))
            continue;

        CSFunctionWriter writer(function);
        writer.GenerateNativeSource(source);
    }

}

void CSClassWriter::GenerateNativeSource(String& sourceOut)
{
    String source = "";

    if (klass_->IsNumberArray())
        return;

    JSBPackage* package = klass_->GetPackage();

    source.AppendWithFormat("ATOMIC_EXPORT_API ClassID csb_%s_%s_GetClassIDStatic()\n{\n", package->GetName().CString(),klass_->GetName().CString());
    source.AppendWithFormat("   return %s::GetClassIDStatic();\n}\n\n", klass_->GetNativeName().CString());

    WriteNativeFunctions(source);

    sourceOut += source;
}

void CSClassWriter::WriteManagedProperties(String& sourceOut)
{
    String source;

    if (klass_->HasProperties())
    {
        Vector<String> pnames;
        klass_->GetPropertyNames(pnames);

        for (unsigned j = 0; j < pnames.Size(); j++)
        {
            JSBProperty* prop = klass_->GetProperty(pnames[j]);

            JSBFunctionType* fType = NULL;
            JSBFunctionType* getType = NULL;
            JSBFunctionType* setType = NULL;

            if (OmitFunction(prop->getter_) || OmitFunction(prop->setter_))
                continue;

            if (prop->getter_ && !prop->getter_->Skip())
            {
                fType = getType = prop->getter_->GetReturnType();
            }
            if (prop->setter_ && !prop->setter_->Skip())
            {
                setType = prop->setter_->GetParameters()[0];

                if (!fType)
                    fType = setType;
                else if (fType->type_->ToString() != setType->type_->ToString())
                    continue;
            }

            if (!fType)
                continue;

            String type = CSTypeHelper::GetManagedTypeString(fType, false);
            String line = ToString("public %s %s\n", type.CString(), prop->name_.CString());
            source += IndentLine(line);
            source += IndentLine("{\n");

            Indent();

            if (prop->getter_)
            {
                source += IndentLine("get\n");
                source += IndentLine("{\n");

                Indent();

                source += IndentLine(ToString("return %s();\n", prop->getter_->GetName().CString()));

                Dedent();

                source += IndentLine("}\n");
            }

            if (prop->setter_)
            {
                source += IndentLine("set\n");
                source += IndentLine("{\n");

                Indent();

                source += IndentLine(ToString("%s(value);\n", prop->setter_->GetName().CString()));

                Dedent();

                source += IndentLine("}\n");
            }


            Dedent();

            source += IndentLine("}\n\n");
        }

    }

    sourceOut += source;

}

bool CSClassWriter::OmitFunction(JSBFunction* function)
{
    if (!function)
        return false;

    // We need to rename GetType
    if (function->GetName() == "GetType")
        return true;

    // avoid vector type for now
    if (function->GetReturnType() && function->GetReturnType()->type_->asVectorType())
        return true;

    Vector<JSBFunctionType*>& parameters = function->GetParameters();

    for (unsigned i = 0; i < parameters.Size(); i++)
    {
        if (parameters[i]->type_->asVectorType())
            return true;
    }

    return false;
}

void CSClassWriter::GenerateManagedSource(String& sourceOut)
{
    String source = "";

    if (klass_->IsNumberArray())
        return;

    Indent();

    source += "\n";
    String line;

    if (klass_->GetBaseClass())
        line = "public partial class " + klass_->GetName() + " : " + klass_->GetBaseClass()->GetName() + "\n";
    else
        line = "public partial class " + klass_->GetName() + "\n";


    source += IndentLine(line);
    source += IndentLine("{\n");

    Indent();

    WriteManagedProperties(source);

    Indent();
    JSBPackage* package = klass_->GetPackage();
    line = "[DllImport (Constants.LIBNAME, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Ansi)]\n";
    source += IndentLine(line);
    line = ToString("public static extern IntPtr csb_%s_%s_GetClassIDStatic();\n", package->GetName().CString(),klass_->GetName().CString());
    source += IndentLine(line);
    source += "\n";
    Dedent();

    // managed functions
    for (unsigned i = 0; i < klass_->functions_.Size(); i++)
    {
        JSBFunction* function = klass_->functions_.At(i);

        if (function->Skip())
            continue;

        if (function->IsDestructor())
            continue;

        if (OmitFunction(function))
            continue;

        CSFunctionWriter fwriter(function);
        fwriter.GenerateManagedSource(source);

    }


    Dedent();

    source += IndentLine("}\n");

    Dedent();

    sourceOut += source;
}


void CSClassWriter::GenerateSource(String& sourceOut)
{

}

}
