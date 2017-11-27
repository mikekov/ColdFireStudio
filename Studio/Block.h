#pragma once

namespace {

	struct Block
	{
		Block(bool& b) : b_(b)
		{
			b_ = true;
		}

		~Block()
		{
			b_ = false;
		}

		bool& b_;
	};

}
