require 'mxx_ru/binary_unittest'

path = 'test/so_5/mchain/limited_no_app_abort'

MxxRu::setup_target(
	MxxRu::BinaryUnittestTarget.new(
		"#{path}/prj.ut.rb",
		"#{path}/prj.rb" )
)
