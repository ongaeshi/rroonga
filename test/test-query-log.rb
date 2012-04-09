# -*- coding: utf-8 -*-
# Copyright (C) 2011  Kouhei Sutou <kou@clear-code.com>
#
# This library is free software; you can redistribute it and/or
# modify it under the terms of the GNU Lesser General Public
# License version 2.1 as published by the Free Software Foundation.
#
# This library is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public
# License along with this library; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

require "cgi"

module QueryLogTest
  module CommandParseTestUtils
    private
    def command(name, parameters)
      Groonga::QueryLog::Command.new(name, parameters)
    end

    def parse_http_path(command, parameters)
      path = "/d/#{command}.json"
      unless parameters.empty?
        uri_parameters = parameters.collect do |key, value|
          [CGI.escape(key.to_s), CGI.escape(value.to_s)].join("=")
        end
        path << "?"
        path << uri_parameters.join("&")
      end
      Groonga::QueryLog::Command.parse(path)
    end

    def parse_command_line(command, parameters)
      command_line = "#{command} --output_type json"
      parameters.each do |key, value|
        if /"| / =~ value
          escaped_value = '"' + value.gsub(/"/, '\"') + '"'
        else
          escaped_value = value
        end
        command_line << " --#{key} #{escaped_value}"
      end
      Groonga::QueryLog::Command.parse(command_line)
    end
  end

  module HTTPCommandParseTestUtils
    private
    def parse(command, parameters={})
      parse_http_path(command, parameters)
    end
  end

  module CommandLineCommandParseTestUtils
    private
    def parse(command, parameters={})
      parse_command_line(command, parameters)
    end
  end

  module SelectCommandParseTests
    include CommandParseTestUtils

    def test_parameters
      select = parse("select",
                     :table => "Users",
                     :filter => "age<=30")
      assert_equal(command("select",
                           "table" => "Users",
                           "filter" => "age<=30",
                           "output_type" => "json"),
                   select)
    end

    def test_scorer
      select = parse("select",
                     :table => "Users",
                     :filter => "age<=30",
                     :scorer => "_score = random()")
      assert_equal("_score = random()", select.scorer)
    end

    def test_to_uri_format
      select = parse("select",
                     :table => "Users",
                     :filter => "age<=30")
      assert_equal("/d/select.json?filter=age%3C%3D30&table=Users",
                   select.to_uri_format)
    end

    def test_to_command_format
      select = parse("select",
                     :table => "Users",
                     :filter => "age<=30")
      assert_equal("select --filter \"age<=30\" " +
                   "--output_type \"json\" --table \"Users\"",
                   select.to_command_format)
    end
  end

  module SelectCommandParseFilterTests
    include CommandParseTestUtils

    def test_parenthesis
      filter = 'geo_in_rectangle(location,' +
                                '"35.73360x139.7394","62614x139.7714") && ' +
               '((type == "たいやき" || type == "和菓子")) && ' +
               'keyword @ "たいやき" &! keyword @ "白" &! keyword @ "養殖"'
      select = parse("select",
                     :table => "Users",
                     :filter => filter)
      assert_equal(['geo_in_rectangle(location,' +
                                     '"35.73360x139.7394","62614x139.7714")',
                     'type == "たいやき"',
                     'type == "和菓子"',
                     'keyword @ "たいやき"',
                     'keyword @ "白"',
                     'keyword @ "養殖"'],
                   select.conditions)
    end

    def test_to_uri_format
      filter = 'geo_in_rectangle(location,' +
                                '"35.73360x139.7394","62614x139.7714") && ' +
               '((type == "たいやき" || type == "和菓子")) && ' +
               'keyword @ "たいやき" &! keyword @ "白" &! keyword @ "養殖"'
      select = parse("select",
                     :table => "Users",
                     :filter => filter)
      assert_equal("/d/select.json?filter=geo_in_rectangle%28location%2C" +
                   "%2235.73360x139.7394%22%2C%2262614x139.7714%22%29+" +
                   "%26%26+%28%28type+" +
                   "%3D%3D+%22%E3%81%9F%E3%81%84%E3%82%84%E3%81%8D%22+" +
                   "%7C%7C+type+%3D%3D+" +
                   "%22%E5%92%8C%E8%8F%93%E5%AD%90%22%29%29+" +
                   "%26%26+keyword+%40+" +
                   "%22%E3%81%9F%E3%81%84%E3%82%84%E3%81%8D%22+%26%21+" +
                   "keyword+%40+%22%E7%99%BD%22+%26%21+" +
                   "keyword+%40+%22%E9%A4%8A%E6%AE%96%22&table=Users",
                   select.to_uri_format)
    end

    def test_to_command_format
      filter = 'geo_in_rectangle(location,' +
                                '"35.73360x139.7394","62614x139.7714") && ' +
               '((type == "たいやき" || type == "和菓子")) && ' +
               'keyword @ "たいやき" &! keyword @ "白" &! keyword @ "養殖"'
      select = parse("select",
                     :table => "Users",
                     :filter => filter)
      assert_equal("select " +
                   "--filter " +
                     "\"geo_in_rectangle(location," +
                     "\\\"35.73360x139.7394\\\",\\\"62614x139.7714\\\") && " +
                     "((type == \\\"たいやき\\\" || " +
                       "type == \\\"和菓子\\\")) && " +
                     "keyword @ \\\"たいやき\\\" &! keyword @ \\\"白\\\" &! " +
                     "keyword @ \\\"養殖\\\"\" " +
                   "--output_type \"json\" --table \"Users\"",
                   select.to_command_format)
    end
  end

  class HTTPSelectCommandParseTest < Test::Unit::TestCase
    include SelectCommandParseTests
    include SelectCommandParseFilterTests
    include HTTPCommandParseTestUtils

    def test_uri_format?
      command = parse("status")
      assert_predicate(command, :uri_format?)
    end

    def test_command_format?
      command = parse("status")
      assert_not_predicate(command, :command_format?)
    end
  end

  class CommandLineSelecCommandParseTest < Test::Unit::TestCase
    include SelectCommandParseTests
    include SelectCommandParseFilterTests
    include CommandLineCommandParseTestUtils

    def test_uri_format?
      command = parse("status")
      assert_not_predicate(command, :uri_format?)
    end

    def test_command_format?
      command = parse("status")
      assert_predicate(command, :command_format?)
    end
  end

  class StatisticOperationParseTest < Test::Unit::TestCase
    def test_context
      operations = statistics.first.operations.collect do |operation|
        [operation[:name], operation[:context]]
      end
      expected = [
        ["filter", "local_name @ \"gsub\""],
        ["filter", "description @ \"string\""],
        ["select", nil],
        ["sort", "_score"],
        ["output", "_key"],
        ["drilldown", "name"],
        ["drilldown", "class"],
      ]
      assert_equal(expected, operations)
    end

    def test_n_records
      operations = statistics.first.operations.collect do |operation|
        [operation[:name], operation[:n_records]]
      end
      expected = [
        ["filter", 15],
        ["filter", 13],
        ["select", 13],
        ["sort", 10],
        ["output", 10],
        ["drilldown", 3],
        ["drilldown", 2],
      ]
      assert_equal(expected, operations)
    end

    private
    def log
      <<-EOL
2011-06-02 16:27:04.731685|5091e5c0|>/d/select.join?table=Entries&filter=local_name+%40+%22gsub%22+%26%26+description+%40+%22string%22&sortby=_score&output_columns=_key&drilldown=name,class
2011-06-02 16:27:04.733539|5091e5c0|:000000001849451 filter(15)
2011-06-02 16:27:04.734978|5091e5c0|:000000003293459 filter(13)
2011-06-02 16:27:04.735012|5091e5c0|:000000003327415 select(13)
2011-06-02 16:27:04.735096|5091e5c0|:000000003411824 sort(10)
2011-06-02 16:27:04.735232|5091e5c0|:000000003547265 output(10)
2011-06-02 16:27:04.735606|5091e5c0|:000000003921419 drilldown(3)
2011-06-02 16:27:04.735762|5091e5c0|:000000004077552 drilldown(2)
2011-06-02 16:27:04.735808|5091e5c0|<000000004123726 rc=0
EOL
    end

    def statistics
      statistics = []
      parser = Groonga::QueryLog::Parser.new
      parser.parse(StringIO.new(log)) do |statistic|
        statistics << statistic
      end
      statistics
    end
  end
end
