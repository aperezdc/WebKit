#!/usr/bin/env ruby
#
# Copyright (c) 2026 Igalia S.L. All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
# 1. Redistributions of source code must retain the above copyright
#    notice, this list of conditions and the following disclaimer.
# 2. Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimer in the
#    documentation and/or other materials provided with the distribution.
#
# THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
# AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
# THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
# PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
# BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
# CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
# SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
# INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
# CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
# ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
# THE POSSIBILITY OF SUCH DAMAGE.

require 'fileutils'
require 'erb'
require 'optparse'
require 'yaml'

def underscorify(s)
  s.gsub(/\b([A-Z]+)([A-Z])/) { $1.downcase + $2 }
    .gsub(/\b([A-Z])/) { $1.downcase }
    .gsub(/(.)([A-Z]+)/, '\1_\2')
    .gsub(/[^a-zA-Z0-9]+/, '_')
    .downcase
end

# def camelCasify(s)
#   head = s.match(/^(\_+).*$/)
#   if head
#     head = head[1]
#   else
#     head = ''
#   end
#
#   tail = s.match(/^.*[^_](\_+)$/)
#   if tail
#     tail = tail[1]
#   else
#     tail = ''
#   end
#
#   middle = s.delete_prefix(head)
#   middle.delete_suffix!(tail)
#
#   head + underscorify(middle).gsub(/_([a-z])/) { $1.upcase } + tail
# end
#
# def PascalCasify(s)
#   head = s.match(/^(\_+).*$/)
#   if head
#     head = head[1]
#   else
#     head = ''
#   end
#
#   tail = s.delete_prefix(head)
#
#   r = camelCasify(tail)
#   if r == ''
#     head
#   else
#     head + r[0].upcase + r[1..]
#   end
# end

class Setting
  attr_accessor :name
  attr_accessor :description
  attr_accessor :type
  attr_accessor :defaultValue
  attr_accessor :condition
  attr_accessor :hidden
  attr_accessor :androidProperty
  attr_accessor :environmentVariables

  def initialize(name, opts)
    @name = name
    @opts = opts
    @description = opts['description']
    @type = opts['type'] || 'String'
    @defaultValue = opts['default']
    @condition = opts['condition']
    @hidden = opts['hidden'] || !@description
    @androidProperty = opts['androidProperty'] || 'debug.WPEWebKit.' + self.nameLower
    @environmentVariables = opts['environmentVariables'] || ['WEBKIT_' + underscorify(@name).upcase]
  end

  def nameLower
    @name[0].downcase + @name[1..]
  end

  def typeUpper
    m = @type.match(/^u?int([\d+])_t$/)
    if m
      if @type[0] == 'u'
        'UInt' + m[1]
      else
        'Int' + m[1]
      end
    else
      @type[0].upcase + @type[1..]
    end
  end
end

def load(path)
  settings = []
  parsed = begin
    YAML.load_file(path)
  rescue ArgumentError => e
    STDERR.puts "error: Could not parse input file: #{e.message}"
    exit 1
  end
  if parsed
    parsed.each do |name, options|
      settings << Setting.new(name, options)
    end
  end
  settings
end

def createTemplate(templateString)
  # Newer versions of ruby deprecate and/or drop passing non-keyword
  # arguments for trim_mode and friends, so we need to call the constructor
  # differently depending on what it expects. This solution is suggested by
  # rubocop's Lint/ErbNewArguments.
  if ERB.instance_method(:initialize).parameters.assoc(:key) # Ruby 2.6+
    ERB.new(templateString, trim_mode:"-")
  else
    ERB.new(templateString, nil, "-")
  end
end

def renderTemplate(templateFile, outputDirectory, settings)
  resultFile = File.join(outputDirectory, File.basename(templateFile, '.erb'))
  tempResultFile = resultFile + '.tmp'

  erb = createTemplate(File.read(templateFile))
  erb.filename = templateFile
  output = erb.result_with_hash({
    settings: settings,
    allConditions: settings.lazy.map { |item| item.condition }.to_set,
    warning: 'THIS FILE WAS AUTOMATICALLY GENERATED, DO NOT EDIT.',
  })
  File.open(tempResultFile, 'w+') do |f|
    f.write(output)
  end
  if (!File.exist?(resultFile) || IO::read(resultFile) != IO::read(tempResultFile))
    FileUtils.move(tempResultFile, resultFile)
  else
    FileUtils.remove_file(tempResultFile)
    FileUtils.uptodate?(resultFile, [templateFile]) or FileUtils.touch(resultFile)
  end
end

def main
  options = {
    :outputDirectory => Dir.getwd,
    :templates => [],
    :settings => nil,
    :testing => false,
  }

  optparse = OptionParser.new do |opts|
    opts.banner = "Usage: #{File.basename($0)} [--outputDir <path>] --template <file> [--template <file>...] settings]"
    opts.separator ''

    opts.on('-o', '--outputDir output', 'directory to generate file in (default: cwd') {
      |outputDir| options[:outputDirectory] = outputDir
    }
    opts.on('-t', '--template input', 'template to use for generation (may be specified multiple times)') {
      |input| options[:templates] << input
    }
    opts.on('--selftest', 'run unit tests') {
      options[:testing] = true
    }
  end

  optparse.parse!

  if options[:testing]
    require 'minitest/autorun'
    return
  end

  if ARGV.size != 1
    puts optparse
    exit 1
  end

  settings = load(ARGV[0])
  FileUtils.mkdir_p(options[:outputDirectory])

  options[:templates].each do |template|
    renderTemplate(template, options[:outputDirectory], settings)
  end
end

### Unit tests ############################################################

require 'minitest/test'

class IdentifierModifyTest < Minitest::Test
  def test_underscorify_noop
    ['', 'oneword', 'two_words', '_leading_underscore', 'trailing_underscore_']
      .each { |item| assert_equal item, underscorify(item) }
  end

  def test_underscorify_leading_capitals
    [
      ['AHardwareBuffer', 'a_hardware_buffer'],
      ['GtkDialogWindow', 'gtk_dialog_window'],
      ['WPEWebView', 'wpe_web_view'],
    ].each { |item| assert_equal item[1], underscorify(item[0]) }
  end

  def test_underscorify_non_alnum_replacement
    [
      [' ', '_'],
      ["\t", '_'],
      ['a b c', 'a_b_c'],
      ['100%valid', '100_valid'],
      ['collapse   multiple', 'collapse_multiple'],
    ].each { |item| assert_equal item[1], underscorify(item[0]) }
  end

  # def test_camelcasify_noop
  #   [
  #     '', 'oneword', 'twoOrMoreWords',
  #     '_leadingUnderscore', 'trailingUnderscore_', '_bothUnderscores_',
  #     '___multipleLeadingUnderscores', 'multipleTrailingUnderscores___',
  #   ].each { |item| assert_equal item, camelCasify(item) }
  # end
  #
  # def test_camelcasify
  #   [
  #     ['snake_case', 'snakeCase'],
  #     ['PascalCase', 'pascalCase'],
  #     ['_under_score_', '_underScore_'],
  #     ['one two three', 'oneTwoThree'],
  #   ].each { |item| assert_equal item[1], camelCasify(item[0]) }
  # end
  #
  # def test_pascalcasify_noop
  #   [
  #     '', 'Noop', 'PascalCase', 'MoreThanTwoWords',
  #     '_LeadingUnderscore', 'TrailingUnderscore_', '_BothUnderscores_',
  #     '___MultipleLeadingUnderscores', 'MultipleTrailingUnderscores___',
  #   ].each { |item| assert_equal item, PascalCasify(item) }
  # end
end

class SettingTest < Minitest::Test
  def test_name_conversions
    s = Setting.new('TestSetting', { 'type' => 'bool' })
    assert_equal 'TestSetting', s.name
    assert_equal 'debug.WPEWebKit.testSetting', s.androidProperty
    assert_equal ['WEBKIT_TEST_SETTING'], s.environmentVariables
  end

  def test_type_conversions
    [
      ['bool', 'Bool'],
      ['string', 'String'],
      ['uint8_t', 'UInt8'], ['int8_t', 'Int8'],
    ].each { |item|
      s = Setting.new('TestSetting', { 'type' => item[0] })
      assert_equal item[1], s.typeUpper
    }
  end

  def test_hidden
    # No description
    assert_equal true, Setting.new('Tweak', { }).hidden
    # Explicitly hidden
    assert_equal true, Setting.new('Tweak', { 'hidden' => true }).hidden
    assert_equal true, Setting.new('Tweak', { 'description' => 'Kwak', 'hidden' => true }).hidden
    # Implicitly shown (has description)
    assert_equal false, Setting.new('Tweak', { 'description' => 'Kwak' }).hidden
  end

  def test_environment_variables_override
    s = Setting.new('TestSetting', { 'environmentVariables' => ['VAR_ONE', 'VAR_TWO'] })
    assert_equal ['VAR_ONE', 'VAR_TWO'], s.environmentVariables
  end

  def test_android_property_override
    s = Setting.new('TestSetting', { 'androidProperty' => 'SomePropertyName' })
    assert_equal 'SomePropertyName', s.androidProperty
  end
end

### Kickoff! ##############################################################
main
