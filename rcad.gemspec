# coding: utf-8
lib = File.expand_path('../lib', __FILE__)
$LOAD_PATH.unshift(lib) unless $LOAD_PATH.include?(lib)
require 'rcad/version'

Gem::Specification.new do |spec|
  spec.name          = "rcad"
  spec.version       = Rcad::VERSION
  spec.authors       = ["Y. Sapir"]
  spec.email         = ["yasapir@gmail.com"]
  spec.description   = %q{Solid CAD programming library}
  spec.summary       = %q{Ruby CAD library}
  spec.homepage      = "http://github.com/sapir/rcad"
# TODO: choose a license:  spec.license       = "MIT"

  spec.files         = `git ls-files`.split($/)
  spec.executables   = spec.files.grep(%r{^bin/}) { |f| File.basename(f) }
  spec.test_files    = spec.files.grep(%r{^(test|spec|features)/})
  spec.require_paths = ["lib"]

  spec.extensions    = ['ext/_rcad/extconf.rb']

  spec.add_development_dependency "bundler", "~> 1.3"
  spec.add_development_dependency "rake"
  spec.add_development_dependency "rake-compiler", "~> 0.8.3"
  spec.add_development_dependency "rice", "~> 1.5.3"
end
