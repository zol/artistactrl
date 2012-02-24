# libartista is a ruby wrapper for artistactrl, a command line utility for 
# controlling USB LCD screens.
# Zoltan Olah (zol@me.com) released under the MIT license in 2007.
require 'yaml'

ARTISTA_CTRL = File.dirname( __FILE__ ) + '/artistactrl'

class Artista
  def self.get_ids
    return run('--get_ids')
  end
  
  def self.set_id(old_id, new_id)
    return run("--set_id \"#{old_id}\" \"#{new_id}\"")
  end
  
  def self.show(image_file, screen_id)
    return run("--show \"#{image_file}\" \"#{screen_id}\"")
  end 
  
  def self.blank_all()
    return run("-r all")
  end
end

def run(args)
  output = `#{ARTISTA_CTRL} #{args}`
  exit_status = $?.exitstatus  
  
  if exit_status == 0 then
    # success
    yaml = YAML::load(output)
    
    return yaml
  else
    raise Exception.new('Error in libartista, output:' + output +
                        'With args:' + args)
  end
  
  nil
end