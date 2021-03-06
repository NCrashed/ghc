test('barton-mangler-bug',
     [when(fast(), skip),
      extra_clean(['Basic.hi',             'Basic.o',
                   'Expected.hi',          'Expected.o',
                   'Main.hi',              'Main.o',
                   'Physical.hi',          'Physical.o',
                   'Plot.hi',              'Plot.o',
                   'PlotExample.hi',       'PlotExample.o',
                   'TypesettingTricks.hi', 'TypesettingTricks.o']),
      omit_ways('debug') # Fails for debug way due to annotation linting timeout
      ],
     multimod_compile_and_run, ['Main', ''])

