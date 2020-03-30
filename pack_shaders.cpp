#include <stdio.h>
#include <vector>

const char* s_var_names[] =
{
	"s_vert_simple",
	"s_frag_simple"
};

const char* s_files[] =
{
	"shaders/vert_simple.spv",
	"shaders/frag_simple.spv",
};

int main()
{
	int num_files = (int)sizeof(s_files) / (int)sizeof(const char*);
	FILE *fp = fopen("shaders_packed.hpp", "w");

	fputs("/* This file is generated by pack_shaders.cpp */ \n", fp);

	for (int i = 0; i < num_files; i++)
	{
		FILE *fheader = fopen(s_files[i], "rb");
		fseek(fheader, 0, SEEK_END);
		size_t size = (size_t)ftell(fheader);
		fseek(fheader, 0, SEEK_SET);
		std::vector<char> buf((size + 3)&(~3));
		fread(buf.data(), 1, size, fheader);
		fclose(fheader);

		size_t num_dwords = buf.size() >> 2;
		unsigned *dwords = (unsigned*)buf.data();

		fprintf(fp, "static unsigned %s[]={\n", s_var_names[i]);
		for (size_t j = 0; j < num_dwords; j++)
		{
			fprintf(fp, "0x%x,", dwords[j]);
			if (j % 10 == 9)
				fputs("\n", fp);
		}
		fputs("};\n\n", fp);
	}

	fclose(fp);

	return 0;
}